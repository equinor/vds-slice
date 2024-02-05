package main

import (
	"fmt"
	"os"
	"strconv"
	"strings"

	"github.com/gin-contrib/gzip"
	"github.com/gin-gonic/gin"
	"github.com/pborman/getopt/v2"
	swaggerFiles "github.com/swaggo/files"
	ginSwagger "github.com/swaggo/gin-swagger"

	"github.com/equinor/vds-slice/api"
	_ "github.com/equinor/vds-slice/docs"
	"github.com/equinor/vds-slice/internal/cache"
	"github.com/equinor/vds-slice/internal/core"
	"github.com/equinor/vds-slice/internal/logging"
	"github.com/equinor/vds-slice/internal/metrics"
)

type opts struct {
	storageAccounts string
	port            uint32
	cacheSize       uint64
	metrics         bool
	metricsPort     uint32
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

func parseAsUint64(fallback uint64, value string) uint64 {
	if len(value) == 0 {
		return fallback
	}
	out, err := strconv.ParseUint(value, 10, 64)
	if err != nil {
		panic(err)
	}

	return out
}

func parseAsString(fallback string, value string) string {
	if len(value) == 0 {
		return fallback
	}
	return value
}

func parseAsBool(fallback bool, value string) bool {
	v, err := strconv.ParseBool(value)
	if err != nil {
		return fallback
	}

	return v
}

func parseopts() opts {
	help := getopt.BoolLong("help", 0, "print this help text")

	opts := opts{
		storageAccounts: parseAsString("", os.Getenv("VDSSLICE_STORAGE_ACCOUNTS")),
		port:            parseAsUint32(8080, os.Getenv("VDSSLICE_PORT")),
		cacheSize:       parseAsUint64(0, os.Getenv("VDSSLICE_CACHE_SIZE")),
		metrics:         parseAsBool(false, os.Getenv("VDSSLICE_METRICS")),
		metricsPort:     parseAsUint32(8081, os.Getenv("VDSSLICE_METRICS_PORT")),
	}

	getopt.FlagLong(
		&opts.storageAccounts,
		"storage-accounts",
		0,
		"Comma-separated list of storage accounts that should be accepted by the API.\n"+
			"Example: 'https://<account1>.blob.core.windows.net,https://<account2>.blob.core.windows.net'\n"+
			"Can also be set by environment variable 'VDSSLICE_STORAGE_ACCOUNTS'",
		"string",
	)

	getopt.FlagLong(
		&opts.port,
		"port",
		0,
		"Port to start server on. Defaults to 8080.\n"+
			"Can also be set by environment variable 'VDSSLICE_PORT'",
		"int",
	)

	getopt.FlagLong(
		&opts.cacheSize,
		"cache-size",
		0,
		"Max size of the response cache. In megabytes. A value of zero effectively\n"+
			"disables caching. Defaults to 0.\n"+
			"Can also be set by environment variable 'VDSSLICE_CACHE_SIZE'",
		"int",
	)

	getopt.FlagLong(
		&opts.metrics,
		"metrics",
		0,
		"Turn on server metrics. Metrics are posted to /metrics using the\n"+
			"prometheus data model. Off by default.\n"+
			"Can also be set by environment variable 'VDSSLICE_METRICS'",
	)

	getopt.FlagLong(
		&opts.metricsPort,
		"metrics-port",
		0,
		"Port to host the /metrics endpoint on. Metrics are always hosted on a\n"+
			"different port than the server itself. This allows for them to be kept\n"+
			"private, if desirable. Defaults to 8081.\n"+
			"Ignored if metrics are not turned on. (see --metrics)\n"+
			"Can also be set by environment variable 'VDSSLICE_METRICS_PORT'",
		"int",
	)

	getopt.Parse()
	if *help {
		getopt.Usage()
		os.Exit(0)
	}

	return opts
}

func setupApp(app *gin.Engine, endpoint *api.Endpoint, metric *metrics.Metrics) {
	app.Use(logging.FormattedLogger())
	app.Use(gin.Recovery())
	app.Use(gzip.Gzip(gzip.BestSpeed))

	seismic := app.Group("/")
	seismic.Use(api.ErrorHandler)

	if metric != nil {
		seismic.Use(metrics.NewGinMiddleware(metric))
	}

	app.GET("/", endpoint.Health)

	seismic.GET("metadata", endpoint.MetadataGet)
	seismic.POST("metadata", endpoint.MetadataPost)

	seismic.GET("slice", endpoint.SliceGet)
	seismic.POST("slice", endpoint.SlicePost)

	seismic.GET("fence", endpoint.FenceGet)
	seismic.POST("fence", endpoint.FencePost)

	attributes := seismic.Group("attributes")
	attributesSurface := attributes.Group("surface")

	attributesSurface.POST("along", endpoint.AttributesAlongSurfacePost)
	attributesSurface.POST("between", endpoint.AttributesBetweenSurfacesPost)

	app.GET("/swagger/*any", ginSwagger.WrapHandler(swaggerFiles.Handler))
	app.LoadHTMLFiles("docs/index.html")
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
		MakeVdsConnection: core.MakeAzureConnection(storageAccounts),
		Cache:             cache.NewCache(opts.cacheSize),
	}

	app := gin.New()
	app.SetTrustedProxies(nil)

	var metric *metrics.Metrics
	if opts.metrics {
		metric = metrics.NewMetrics()
		/*
		 * Host the /metrics endpoint on a different app instance. This is needed
		 * in order to serve it on a different port, while also giving some benefits
		 * such that our main app server's logs doesn't get polluted by tools that
		 * are continually scarping the /metrics endpoint. I.e. Grafana.
		 */
		metricsApp := gin.New()
		metricsApp.SetTrustedProxies(nil)

		metricsApp.Use(gin.Recovery())
		metricsApp.GET("metrics", metrics.NewGinHandler(metric))

		go func() {
			metricsApp.Run(fmt.Sprintf(":%d", opts.metricsPort))
		}()
	}

	setupApp(app, &endpoint, metric)
	app.Run(fmt.Sprintf(":%d", opts.port))
}
