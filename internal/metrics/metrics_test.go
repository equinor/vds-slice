package metrics

import (
	"testing"

	"github.com/stretchr/testify/require"
)

func TestExtractStorageAccounts(t *testing.T) {
	testcases := []struct {
		name     string
		urls     []string
		expected []string
	}{
		{
			name:     "One good vds URL",
			urls:     []string{"https://account.blob.core.windows.net/container/path"},
			expected: []string{"account.blob.core.windows.net"},
		},
		{
			name:     "Empty vds URL",
			urls:     []string{""},
			expected: []string{""},
		},
		{
			name:     "Bad vds URL",
			urls:     []string{"::??::"},
			expected: []string{"::??::"},
		},
		{
			name:     "Two same storage accounts",
			urls:     []string{"https://example.com/path2", "http://example.com/path1"},
			expected: []string{"example.com"},
		},
		{
			name:     "Two different storage accounts",
			urls:     []string{"https://cat2.org/path", "https://cat1.org/path"},
			expected: []string{"cat1.org", "cat2.org"},
		},
		{
			name:     "Good and bad vds URLs",
			urls:     []string{"ht^tp://illegal.com", "https://feed.cat"},
			expected: []string{"ht^tp://illegal.com", "feed.cat"},
		},
	}

	for _, testcase := range testcases {
		storageAccounts := extractStorageAccounts(testcase.urls)
		require.ElementsMatch(t, storageAccounts, testcase.expected, "[case: %v]", testcase.name)
	}
}
