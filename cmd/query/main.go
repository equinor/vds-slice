package main

import (
	"fmt"
	"os"

	"github.com/gin-gonic/gin"
	"github.com/pborman/getopt/v2"

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

func main() {
	opts := parseopts()

	endpoint := api.Endpoint{
		StorageURL: opts.storageURL,
	}

	app := gin.Default()
	app.GET("/", endpoint.Health)
	app.GET( "slice", endpoint.SliceGet)
	app.POST("slice", endpoint.SlicePost)
	app.GET( "slice/metadata", endpoint.SliceMetadataGet)
	app.POST("slice/metadata", endpoint.SliceMetadataPost)

	app.Run(fmt.Sprintf(":%s", opts.port))
}
