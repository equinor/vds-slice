package metrics


import (
	"time"
	"strconv"

	"github.com/gin-gonic/gin"
	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/client_golang/prometheus/promhttp"
)

const (
	ms = 0.001
	s  = 1
	m  = 60 * s
)

type metrics struct {
	registry *prometheus.Registry

	// Custom metics
	requestDurations *prometheus.HistogramVec
}

/** Create a new metric instance
 *
 * The metrics are applied to a new non-global prometheus registry
 */
func NewMetrics() *metrics {
	registry := prometheus.NewRegistry()
	metrics := &metrics{
		registry: registry,

		requestDurations: prometheus.NewHistogramVec(prometheus.HistogramOpts{
			Name:    "vdsslice_durations_histogram_seconds",
			Help:    "VDSslice latency distributions.",
			Buckets: []float64{100*ms, 500*ms, 1*s, 2*s, 5*s, 20*s, 1*m, 2*m},
		}, []string{"path", "status"}),
	}

	registry.MustRegister(metrics.requestDurations)

	return metrics;
}

/** New gin middleware for writing prometheus metrics */
func NewGinMiddleware(metrics *metrics) gin.HandlerFunc {
	return func(ctx *gin.Context) {
		start := time.Now()
		ctx.Next()

		go func() {
			path   := ctx.Request.URL.Path
			status := strconv.Itoa(ctx.Writer.Status())

			duration := time.Since(start).Seconds()
			metrics.requestDurations.WithLabelValues(
				path,
				status,
			).Observe(duration)
		}()
	}
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
