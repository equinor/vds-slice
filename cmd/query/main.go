package main

import (
	"fmt"
	"os"

	"github.com/gin-gonic/gin"
	"github.com/pborman/getopt/v2"
	"github.com/swaggo/gin-swagger"
	"github.com/swaggo/files"

	_ "github.com/equinor/vds-slice/docs"
	"github.com/equinor/vds-slice/api"
)

type opts struct {
	storageURL string
	port       string
}

func parseopts() opts {
	help := getopt.BoolLong("help", 0, "print this help text")
	opts := opts{
		storageURL: os.Getenv("STORAGE_URL"),
		port:       "8080",
	}

	getopt.FlagLong(
		&opts.storageURL,
		"storage-url",
		0,
		"Storage URL, e.g. https://<account>.blob.core.windows.net",
		"string",
	)
	opts.port = "8080"
	getopt.FlagLong(
		&opts.port,
		"port",
		0,
		"Port to start server on. Defaults to 8080",
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

	endpoint := api.Endpoint{
		StorageURL: opts.storageURL,
		Protocol:   "azure://",
	}

	app := gin.Default()
	app.Use(api.ErrorHandler)
	app.GET("/", endpoint.Health)

	app.GET( "metadata", endpoint.MetadataGet)
	app.POST("metadata", endpoint.MetadataPost)

	app.GET( "slice", endpoint.SliceGet)
	app.POST("slice", endpoint.SlicePost)

	app.GET("fence", endpoint.FenceGet)
	app.POST("fence", endpoint.FencePost)

	app.GET("/swagger/*any", ginSwagger.WrapHandler(swaggerFiles.Handler))
	app.Run(fmt.Sprintf(":%s", opts.port))
}
