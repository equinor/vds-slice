package main

import (
	"os"
	"errors"
	"log"
	"fmt"
	"net"
	"strconv"
	"strings"

	"github.com/pborman/getopt/v2"
	"google.golang.org/grpc"

	pb "github.com/equinor/vds-slice/internal/grpc"
)

type opts struct {
	ports string
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
		ports: parseAsString("8082", os.Getenv("VDSSLICE_WORKER_PORTS")),
	}
	getopt.FlagLong(
		&opts.ports,
		"ports",
		0,
		"Choice of ports to run the worker on. When presented with " +
		"multiple options, the worker will choose one that is available. " +
		"Seperate options by ',' and use '-' to specify a portrange. E.g. " +
		"':8080-8083,:8087,:8089' will be interpreted as " +
		"8082,8083,8084,8087,8089. All on localhost." +
		"Defaults to :8082.\n" +
		"Defaults to 8082.\n" +
		"Can also be set by environment variable 'VDSSLICE_WORKER_PORTS'",
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

func listen(ports []string) (net.Listener, error) {
	for _, port := range ports {
		listener, err := net.Listen("tcp", port)
		if err == nil {
			return listener, nil
		}
	}

	return nil, errors.New("no available ports")
}

func main() {
	opts := parseopts()

	ports, err := parsePorts(opts.ports)
	if err != nil {
		log.Fatalf(
			"Invalid value for option 'addresses': (was: %v), (err: %v)", 
			opts.ports,
			err,
		)
	}
	
	lis, err := listen(ports)
	if err != nil {
		log.Fatalf("failed to listen: %v", err)
	}

	server := grpc.NewServer()
	pb.RegisterOneseismicServer(server, &pb.Worker{})

	if err := server.Serve(lis); err != nil {
		log.Fatalf("failed to serve: %v", err)
	}
}
