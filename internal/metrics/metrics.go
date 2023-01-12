package metrics


import (
	"github.com/gin-gonic/gin"
	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/client_golang/prometheus/promhttp"
)

type metrics struct {
	registry *prometheus.Registry
}

/** Create a new metric instance
 *
 * The metrics are applied to a new non-global prometheus registry
 */
func NewMetrics() *metrics {
	registry := prometheus.NewRegistry()
	return &metrics{registry: registry}
}

/** New gin middleware for writing prometheus metrics */
func NewGinMiddleware(metrics *metrics) gin.HandlerFunc {
	return func(ctx *gin.Context) { }
}

/** New gin handler for prometheus
 *
 * A tiny helper that sets up a handle for promethus and wraps it in
 * a gin handler function such that it can be applied to a gin app.
 */
func NewGinHandler(metrics *metrics) gin.HandlerFunc {
	return gin.WrapH(
		promhttp.HandlerFor(
			metrics.registry,
			promhttp.HandlerOpts{
				EnableOpenMetrics: true,
				Registry: metrics.registry,
			},
		),
	)
}
