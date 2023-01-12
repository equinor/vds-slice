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
	kb = 1024
	mb = 1024*kb
)

type metrics struct {
	registry *prometheus.Registry

	// Custom metics
	requestDurations *prometheus.HistogramVec
	responseSizes    *prometheus.HistogramVec
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
		}, []string{"path", "status", "cachehit"}),

		responseSizes: prometheus.NewHistogramVec(prometheus.HistogramOpts{
			Name:    "vdsslice_response_sizes_histogram_bytes",
			Help:    "VDSslice response size distributions.",
			Buckets: []float64{100*kb, 1*mb, 5*mb, 10*mb, 20*mb, 50*mb, 100*mb, 200*mb},
		}, []string{"path", "status"}),
	}

	registry.MustRegister(metrics.requestDurations)
	registry.MustRegister(metrics.responseSizes)

	return metrics;
}

/** New gin middleware for writing prometheus metrics */
func NewGinMiddleware(metrics *metrics) gin.HandlerFunc {
	return func(ctx *gin.Context) {
		start := time.Now()
		ctx.Next()

		go func() {
			path     := ctx.Request.URL.Path
			status   := strconv.Itoa(ctx.Writer.Status())
			size     := float64(ctx.Writer.Size())
			cachehit := strconv.FormatBool(ctx.GetBool("cache-hit"))
			duration := time.Since(start).Seconds()

			metrics.requestDurations.WithLabelValues(
				path,
				status,
				cachehit,
			).Observe(duration)

			metrics.responseSizes.WithLabelValues(path, status).Observe(size)
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
