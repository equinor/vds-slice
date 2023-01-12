package main

import (
	"fmt"
	"os"
	"strings"
	"strconv"

	"github.com/gin-gonic/gin"
	"github.com/pborman/getopt/v2"
	"github.com/swaggo/gin-swagger"
	"github.com/swaggo/files"
	"github.com/gin-contrib/gzip"

	_ "github.com/equinor/vds-slice/docs"
	"github.com/equinor/vds-slice/api"
	"github.com/equinor/vds-slice/internal/vds"
	"github.com/equinor/vds-slice/internal/cache"
	"github.com/equinor/vds-slice/internal/logging"
)

type opts struct {
	storageAccounts string
	port            uint32
	cacheSize       uint32
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
		storageAccounts: parseAsString("",   os.Getenv("VDSSLICE_STORAGE_ACCOUNTS")),
		port:            parseAsUint32(8080, os.Getenv("VDSSLICE_PORT")),
		cacheSize:       parseAsUint32(0,    os.Getenv("VDSSLICE_CACHE_SIZE")),
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
		&opts.cacheSize,
		"cache-size",
		0,
		"Max size of the response cache. In megabytes. A value of zero effectively\n" +
		"disables caching. Defaults to 0.\n" +
		"Can also be set by environment variable 'VDSSLICE_CACHE_SIZE'",
		"int",
	)

	getopt.Parse()
	if *help {
		getopt.Usage()
		os.Exit(0)
	}

	return opts
}

// @title        VDS-slice API
// @version      0.0
// @description  Serves seismic slices and fences from VDS files.
// @contact.name Equinor ASA
// @contact.url  https://github.com/equinor/vds-slice/issues
// @license.name GNU Affero General Public License
// @license.url  https://www.gnu.org/licenses/agpl-3.0.en.html
// @schemes      https
func main() {
	opts := parseopts()

	storageAccounts := strings.Split(opts.storageAccounts, ",")
	
	endpoint := api.Endpoint{
		MakeVdsConnection: vds.MakeAzureConnection(storageAccounts),
		Cache:             cache.NewCache(opts.cacheSize),
	}

	app := gin.New()
	app.Use(logging.FormattedLogger())
	app.Use(gin.Recovery())
	app.Use(gzip.Gzip(gzip.BestSpeed))

	app.GET("/", endpoint.Health)

	app.GET("metadata", api.ErrorHandler, endpoint.MetadataGet)
	app.POST("metadata", api.ErrorHandler, endpoint.MetadataPost)

	app.GET("slice", api.ErrorHandler, endpoint.SliceGet)
	app.POST("slice", api.ErrorHandler, endpoint.SlicePost)

	app.GET("fence", api.ErrorHandler, endpoint.FenceGet)
	app.POST("fence", api.ErrorHandler, endpoint.FencePost)

	app.GET("/swagger/*any", ginSwagger.WrapHandler(swaggerFiles.Handler))
	app.Run(fmt.Sprintf(":%d", opts.port))
}
