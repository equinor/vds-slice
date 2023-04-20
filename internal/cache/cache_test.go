package cache

import (
	"testing"
	"time"

	"github.com/google/uuid"
)

/** Makes sure that our cache is not growing indefinitely.
 *
 *  There was a bug with us running out of memory which went unnoticed before.
 *  Test mostly tests ristretto functionality, so it can be nuked if it ever
 *  creates trouble.
 */
func TestRistrettoCacheMaxSize(t *testing.T) {
	/** CacheEntry with a memory footprint of exactly 1 KB
	 *
	 * The true size (in memory) is given by the size of the struct itself,
	 * which for cacheEntry is 48 bytes plus the size of the two buffers. I.e:
	 *
	 * unsafe.Sizeof(entry) + len(entry.Data) + len(entry.Metadata) =
	 * 48                   + 512             + 464                 = 1024
	 */
	entry := NewCacheEntry(
		make([][]byte, 512),
		make([]byte, 464),
	)

	cacheSize := 1 * 1024 * 1024 // 1 MB
	maxEntries := cacheSize / entry.Size();

	cache := NewRistrettoCache(uint32(cacheSize))

	overflow := 100
	var keys []string
	for i := 0; i < maxEntries + overflow; i++ {
		key := uuid.New().String()
		cache.Set(key, entry)
		keys = append(keys, key)
	}

	/*
	 * The ristretto cache is highly concurrent (but safe) which means that the
	 * eviction process isn't neccesarly done when Set() returns. This is a bit
	 * unfortunate for this test as we have no good way of waiting for the
	 * eviction to complete.
	 *
	 * This sleep give it enough time to complete all eviction jobs (goroutines) before
	 * we start reading out entries again. This is a poor solution and may cause the
	 * test to be flaky (e.g. on slow systems) but I see no better options.
	 */
	time.Sleep(1 * time.Second)

	var hits int
	for _, key := range(keys) {
		_, hit := cache.Get(key)
		if hit {
			hits++
		}
	}

	if hits != maxEntries {
		t.Fatalf(
			"Expected to find %d entries in cache, found %d. Note that this test might " +
			"be flaky due to internal concurrency in ristretto",
			maxEntries,
			hits,
		)
	}
}
