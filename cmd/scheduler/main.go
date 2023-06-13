package main

import (
	"errors"
	"fmt"
	"log"
	"os"
	"strconv"
	"strings"

	"github.com/gin-contrib/gzip"
	"github.com/gin-gonic/gin"
	"github.com/pborman/getopt/v2"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
	"google.golang.org/grpc/resolver"

	"github.com/equinor/vds-slice/api"
	pb "github.com/equinor/vds-slice/internal/grpc"
	"github.com/equinor/vds-slice/internal/logging"
	"github.com/equinor/vds-slice/internal/vds"
)
const (
	scheme      = "grpc"
	serviceName = "vds-slice"
)
type opts struct {
	storageAccounts  string
	port             uint32
	workerAddresses  string
	jobs             uint32
}

func parseAsUint32(fallback uint32, value string) uint32 {
	if len(value) == 0 {
		return fallback
	}
	out, err := strconv.ParseUint(value, 10, 32)
	if err != nil {
		panic(err)
	}

	return uint32(out)
}

func parseAsString(fallback string, value string) string {
	if len(value) == 0 {
		return fallback
	}
	return value
}

func parseopts() opts {
	help := getopt.BoolLong("help", 0, "print this help text")
	
	opts := opts{
		storageAccounts: parseAsString("",      os.Getenv("VDSSLICE_STORAGE_ACCOUNTS")),
		port:            parseAsUint32(8080,    os.Getenv("VDSSLICE_PORT")),
		jobs:            parseAsUint32(32,      os.Getenv("VDSSLICE_JOBS")),
		workerAddresses: parseAsString(":8080", os.Getenv("VDSSLICE_WORKER_ADDRESSES")),
	}

	getopt.FlagLong(
		&opts.storageAccounts,
		"storage-accounts",
		0,
		"Comma-separated list of storage accounts that should be accepted by the API.\n" +
		"Example: 'https://<account1>.blob.core.windows.net,https://<account2>.blob.core.windows.net'\n" +
		"Can also be set by environment variable 'VDSSLICE_STORAGE_ACCOUNTS'",
		"string",
	)

	getopt.FlagLong(
		&opts.port,
		"port",
		0,
		"Port to start server on. Defaults to 8080.\n" +
		"Can also be set by environment variable 'VDSSLICE_PORT'",
		"int",
	)
	
	getopt.FlagLong(
		&opts.jobs,
		"jobs",
		0,
		"Number of jobs to split request in. Defaults to 32.\n" +
		"Can also be set by environment variable 'VDSSLICE_JOBS'",
		"int",
	)
	
	getopt.FlagLong(
		&opts.workerAddresses,
		"worker-addresses",
		0,
		"Choice of ports to run the worker on. When presented with " +
		"multiple options, the worker will choose one that is available. " +
		"Seperate options by ',' and use ':' to specify a range. E.g. " +
		"'localhost:8080-8083,localhost:8087,:8089' will be interpreted as " +
		"8082,8083,8084,8087,8089. All on localhost." +
		"Defaults to :8082.\n" +
		"Can also be set by environment variable 'VDSSLICE_WORKER_ADDRESSES'",
		"string",
	)
	
	getopt.Parse()
	if *help {
		getopt.Usage()
		os.Exit(0)
	}

	return opts
}

// TODO this is duplicated in both worker and scheduler main, move to some util package
// TODO this logic is currently very fragile with little edgecase handling
func parsePorts(in string) ([]string, error) {
	var out []string
	
	for _, sequence := range strings.Split(in, ",") {
		host, ports, _ := strings.Cut(sequence, ":")
		
		portRange := strings.Split(ports, "-")

		if len(portRange) == 1 {
			portRange = append(portRange, portRange[0])
		}

		if len(portRange) != 2 {
			return nil, errors.New("invalid range")
		}

		begin, err := strconv.ParseUint(portRange[0], 10, 32)
		if err != nil {
			return nil, err
		}
		end, err := strconv.ParseUint(portRange[1], 10, 32)
		if err != nil {
			return nil, err
		}

		for port := begin; port <= end; port++ {
			out = append(out, fmt.Sprintf("%s:%d", host, port))
		}
	}

	return out, nil
}

func main() {
	opts := parseopts()

	ports, err := parsePorts(opts.workerAddresses)
	if err != nil {
		log.Fatalf(
			"invalid value for option 'addresses': (was: %v), (err: %v)",
			opts.workerAddresses,
			err,
		)
	}

	rb := pb.NewResolverBuilder(scheme, serviceName, ports)
	resolver.Register(rb)

	storageAccounts := strings.Split(opts.storageAccounts, ",")
	
	conn, err := grpc.Dial(
		fmt.Sprintf("%s:///%s", scheme, serviceName), 
		grpc.WithTransportCredentials(insecure.NewCredentials()),
		grpc.WithDefaultServiceConfig(`{"loadBalancingConfig": [{"round_robin":{}}]}`),
	)
	if err != nil {
		log.Fatalf("did not connect: %v", err)
	}
	defer conn.Close()
	client := pb.NewOneseismicClient(conn)

	httpRoutes := api.NewHttpRoutes(
		vds.MakeAzureConnection(storageAccounts),
		pb.NewScheduler(client, int(opts.jobs)),
	)

	app := gin.New()
	app.SetTrustedProxies(nil)

	app.Use(logging.FormattedLogger())
	app.Use(gin.Recovery())
	app.Use(gzip.Gzip(gzip.BestSpeed))

	seismic := app.Group("/")
	seismic.Use(api.ErrorHandler)

	seismic.POST("fence", httpRoutes.FencePost)
	
	app.Run(fmt.Sprintf(":%d", opts.port))
}
