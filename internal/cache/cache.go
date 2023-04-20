package cache

import (
	"bytes"
	"crypto/sha1"
	"encoding/gob"
	"fmt"

	"unsafe"
	"github.com/dgraph-io/ristretto"
)

type CacheEntry struct {
	data     [][]byte
	metadata []byte
}

func (c *CacheEntry) Data() [][]byte {
	return c.data
}

func (c *CacheEntry) Metadata() []byte {
	return c.metadata
}

func (c *CacheEntry) Size() int {
	return len(c.data) + len(c.metadata) + int(unsafe.Sizeof(*c))
}

func NewCacheEntry(data [][]byte, metadata []byte) CacheEntry {
	return CacheEntry{ data: data, metadata: metadata }
}

type Cache interface {
	Get(string) (CacheEntry, bool)
	Set(string, CacheEntry)
}

type RistrettoCache struct {
	ristretto.Cache
}
func (c *RistrettoCache) Set(key string, val CacheEntry) {
	c.Cache.Set(key, val, int64(val.Size()))
}
func (c *RistrettoCache) Get(key string) (val CacheEntry, hit bool) {
	v, hit := c.Cache.Get(key)
	if hit {
		val = v.(CacheEntry)
	}
	return val, hit;
}

func NewRistrettoCache(cacheSize uint32) *RistrettoCache {
	/**  Maxcost and NumCounters
	 *
	 * This Ristretto cache is configured with a max size in bytes (the
	 * MaxCost), rather than max number of entries. This gives us more control
	 * over the memory-footprint of the cache, which is very useful when
	 * running the server in environments with limited memory, such as a
	 * container.
	 *
	 * It recommended to set NumCounter to approx 10x the expected number of
	 * entries when the cache is full [1]. A conservative size estimate (in
	 * bytes) is that entries (slices or fences) are on average 1MB. That gives
	 * us an estimate of max number of entries of cacheSize / 1MB.
	 *
	 * [1] https://github.com/dgraph-io/ristretto#Config
	 */
	avgEntrySize := 1 * 1024 * 1024
	cache, err := ristretto.NewCache(&ristretto.Config{
		NumCounters:        10 * int64(cacheSize) / int64(avgEntrySize),
		MaxCost:            int64(cacheSize),
		BufferItems:        64,
		/*
		 * By default Ristretto will add an internal cost to the cost set when
		 * calling Set(). This cost is essentially the size of the struct
		 * (excluding members). Which is a cost that we have already taken into
		 * account. Thus we disable the internal cost to not count the same
		 * cost/size twice.
		 */
		IgnoreInternalCost: true,
	})
	if err != nil {
		panic(fmt.Errorf("failed to create cache, err: %v", err))
	}

	return &RistrettoCache{ Cache: *cache }
}

type NoCache struct {}

func (c *NoCache) Get(key string) (CacheEntry, bool) {
	return CacheEntry{}, false
}

func (c *NoCache) Set(key string, val CacheEntry) {}

func NewNoCache() *NoCache {
	return &NoCache{}
}

/** Return a new cache with maxsize of 'cachesize' (megabytes)
 *
 *  Requesting a new cache with size zero will return a 'NoCache' which behaves
 *  like a cache, but does not cache anything. I.e. it will always be empty and
 *  will have a 100% cache misses.
 */
func NewCache(cachesize uint32) Cache {
	if cachesize == 0 {
		return NewNoCache()
	}
	return NewRistrettoCache(cachesize * 1024 * 1024)
}

/** Compute a sha1 hash of any object
 *
 * The sha1 is cryptographically broken and hence this hasher should not be
 * used for security. Rather it's intended to compute hashes that can be used
 * as keys in a cache.
 */
func Hash(val interface{}) (string, error) {
	/*
	 * Ideally the buffer and the encoder should be reused for more efficient
	 * memory-allocation, but that would not be thread-safe and the effort to
	 * make it so out weighs the benefit.
	 */
	var buffer bytes.Buffer

	encoder := gob.NewEncoder(&buffer)
	err := encoder.Encode(val)
	if (err != nil) {
		return "", err
	}
	return fmt.Sprintf("%x", sha1.Sum(buffer.Bytes())), nil
}
