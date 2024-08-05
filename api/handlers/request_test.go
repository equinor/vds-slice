package handlers

import (
	"testing"

	"github.com/stretchr/testify/require"
)

func newRequestedResource(
	vds []string,
	sas []string,
) RequestedResource {
	return RequestedResource{
		Vds: vds,
		Sas: sas,
	}
}

func TestExtractSasFromUrl(t *testing.T) {

	testCases := []struct {
		request     RequestedResource
		expected    []string
		shouldError bool
	}{
		{
			request: newRequestedResource(
				[]string{"../../testdata/well_known/well_known_default.vds"},
				[]string{"sastoken1"},
			),
			expected:    []string{"sastoken1"},
			shouldError: false,
		},
		{
			request: newRequestedResource(
				[]string{"../../testdata/well_known/well_known_default.vds?sastoken2"},
				[]string{""},
			),
			expected:    []string{"sastoken2"},
			shouldError: false,
		},
		{
			request: newRequestedResource(
				[]string{"../../testdata/well_known/well_known_default.vds?sastoken2"},
				[]string{"sastoken1"},
			),
			expected:    []string{"Signed urls are not accepted when providing sas-tokens. Vds url nr 1 is signed"},
			shouldError: true,
		},
		{
			request: newRequestedResource(
				[]string{"../../testdata/well_known/well_known_default.vds"},
				[]string{""},
			),
			expected:    []string{"No valid Sas token found at the end of vds url nr 1"},
			shouldError: true,
		},
		{
			request: newRequestedResource(
				[]string{
					"../../testdata/well_known/well_known_default.vds",
					"../../testdata/well_known/well_known_default.vds"},
				[]string{"sastoken1", "sastoken2"},
			),
			expected:    []string{"sastoken1", "sastoken2"},
			shouldError: false,
		},
		{
			request: newRequestedResource(
				[]string{
					"../../testdata/well_known/well_known_default.vds?sastoken1",
					"../../testdata/well_known/well_known_default.vds?sastoken2"},
				[]string{"", ""},
			),
			expected:    []string{"Signed urls are not accepted when providing sas-tokens. Vds url nr 1 is signed"},
			shouldError: true,
		},
		{
			request: newRequestedResource(
				[]string{
					"../../testdata/well_known/well_known_default.vds?sastoken1",
					"../../testdata/well_known/well_known_default.vds?sastoken2"},
				[]string{""},
			),
			expected:    []string{"sastoken1", "sastoken2"},
			shouldError: false,
		},

		{
			request: newRequestedResource(
				[]string{
					"../../testdata/well_known/well_known_default.vds?sastoken1",
					"../../testdata/well_known/well_known_default.vds"},
				[]string{""},
			),
			expected:    []string{"No valid Sas token found at the end of vds url nr 2"},
			shouldError: true,
		},
	}

	for _, testCase := range testCases {
		err := testCase.request.NormalizeConnection()
		if testCase.shouldError {
			require.ErrorContains(t, err, testCase.expected[0])
		} else {
			require.NoError(t, err)
			for i := 0; i < len(testCase.expected); i++ {
				require.Equal(t, testCase.expected[i], testCase.request.Sas[i])
			}
		}
	}
}

func TestPortPresenceInURL(t *testing.T) {

	testCases := []struct {
		request  RequestedResource
		expected []string
	}{
		{
			request: newRequestedResource(
				[]string{"https://account.blob.core.windows.net:443/container/blob"},
				[]string{"sastoken1"},
			),
			expected: []string{"https://account.blob.core.windows.net/container/blob"},
		},
		{
			request: newRequestedResource(
				[]string{"https://account.blob.core.windows.net:443/container/blob?sastoken1"},
				[]string{""},
			),
			expected: []string{"https://account.blob.core.windows.net/container/blob"},
		},
		{
			request: newRequestedResource(
				[]string{
					"https://account.blob.core.windows.net:443/container/blob",
					"https://account.blob.core.windows.net:443/container/blob"},
				[]string{"sastoken1", "sastoken2"},
			),
			expected: []string{
				"https://account.blob.core.windows.net/container/blob",
				"https://account.blob.core.windows.net/container/blob"},
		},
		{
			request: newRequestedResource(
				[]string{
					"https://account.blob.core.windows.net:443/container/blob?sastoken1",
					"https://account.blob.core.windows.net:443/container/blob?sastoken2"},
				[]string{""},
			),
			expected: []string{
				"https://account.blob.core.windows.net/container/blob",
				"https://account.blob.core.windows.net/container/blob"},
		},
		{
			request: newRequestedResource(
				[]string{
					"https://account.blob.core.windows.net/container/blob?sastoken1",
					"https://account.blob.core.windows.net:443/container/blob?sastoken2"},
				[]string{""},
			),
			expected: []string{
				"https://account.blob.core.windows.net/container/blob",
				"https://account.blob.core.windows.net/container/blob"},
		},
	}

	for _, testCase := range testCases {
		err := testCase.request.NormalizeConnection()
		require.NoError(t, err)
		for i := 0; i < len(testCase.expected); i++ {
			require.Equal(t, testCase.expected[i], testCase.request.Vds[i])
		}
	}
}
