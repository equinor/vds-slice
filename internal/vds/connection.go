package vds

import (
	"context"
	"fmt"
	"strings"
	"net/url"

	"github.com/Azure/azure-sdk-for-go/sdk/storage/azblob/blob"
)

func equalsOneOf(parameter string, values []string) bool {
	for _, value := range values {
		if strings.EqualFold(parameter, value) {
			return true
		}
	}
	return false
}

func srtContainsObject(srt string) bool {
	return strings.Contains(srt, "o");
}

func srtContainsContainer(srt string) bool {
	return strings.Contains(srt, "c");
}

/** Validate 'srt' (Signed Resource Types)
 *
 * Validate that the srt parameter in the sas token contains both 'c'
 * (container) and 'o' (object/blob). This is the minimum set of resource types
 * needed by OpenVDS to read a VDS from Blob Store.
 *
 * Only Account SAS use the srt field, while Service SAS and User Delegation
 * SAS use Signed Resource (sr). In that case there is nothing to check for and
 * this function will return nil.
 */
func validateSasSrt(connection *AzureConnection) error {
	query, err := url.ParseQuery(connection.sas)
	if err != nil {
		return NewInvalidArgument(fmt.Sprintf(
			"illegal sas-token, was: '%s',  err: %v",
			connection.sas,
			err,
		))
	}

	if !query.Has("srt") {
		return nil
	}

	srt := query.Get("srt")
	if srtContainsObject(srt) && srtContainsContainer(srt) {
		return nil
	}
	return NewInvalidArgument(fmt.Sprintf(
		"invalid sas-token, expected 'c' and 'o' in 'srt', found: '%s'",
		srt,
	))
}

type Connection interface {
	Url()              string
	ConnectionString() string
	IsAuthorizedToRead()    bool
}

type AzureConnection struct {
	blobPath  string
	container string
	host      string
	sas       string
}

func (c *AzureConnection) Url() string {
	return fmt.Sprintf("azure://%s/%s", c.container, c.blobPath)
}

func (c *AzureConnection) ConnectionString() string {
	return fmt.Sprintf("BlobEndpoint=https://%s;SharedAccessSignature=?%s",
		c.host,
		c.sas,
	)
}

/** Verify that the connection has enough access to read a VDS from Blob Store
 *
 * This function does a HEAD request to the VolumeDataLayout in the VDS, in
 * form of a 'get blob properties' request [1] in order to verify read access,
 * and that the token is generally valid.
 *
 * The HEAD request itself is not sufficient if the SAS token happens to be a
 * Service- or User Delegation SAS. In that case we also need to check if the
 * Signed Resource ('sr') parameter contains either 'c' (container) or 'd'
 * (directory). The presence of either 'c' or 'd' confirms that the SAS-token is
 * valid not only for the VolumeDataLayout blob, but all blobs in the same and
 * subsequent directories relative to the VolumeDataLayout blob.
 *
 * [1] https://learn.microsoft.com/en-us/rest/api/storageservices/get-blob-properties
 */
func (c *AzureConnection) IsAuthorizedToRead() bool {
	query, err := url.ParseQuery(c.sas)
	if err != nil {
		return false
	}

	if query.Has("sr") && !equalsOneOf(query.Get("sr"), []string{"c", "d"}) {
		return false
	}

	client, err := blob.NewClientWithNoCredential(
		fmt.Sprintf("https://%s/%s/%s/VolumeDataLayout?%s",
			c.host,
			c.container,
			c.blobPath,
			c.sas,
		),
		nil,
	)

	if err != nil {
		return false
	}

	_, err = client.GetProperties(context.Background(), nil);
	return err == nil
}

func NewAzureConnection(
	blobPath  string,
	container string,
	host      string,
	sas       string,
) *AzureConnection {
	return &AzureConnection{
		blobPath:  blobPath,
		container: container,
		host:      host,
		sas:       sas,
	}
}

type FileConnection struct {
	url string
}

func (f *FileConnection) Url() string {
	return f.url
}

func (f *FileConnection) ConnectionString() string {
	return ""
}

func (c *FileConnection) IsAuthorizedToRead() bool {
	return true
}

func NewFileConnection(path string) *FileConnection {
	return &FileConnection{ url: path }
}

/*
 * Sanitize path and make the input path into a *url.URL type
 *
 * OpenVDS segfaults if the blobpath ends on a trailing slash. We don't want to
 * propagate that behaviour to the user in any way, so we need to explicitly
 * sanitize the input and strip trailing slashes before passing them on to
 * OpenVDS.
 */
func makeUrl(path string) (*url.URL, error) {
	path = strings.TrimSuffix(path, "/")
	return url.Parse(path)
}

func isAllowed(allowlist []*url.URL, requested *url.URL) error {
	for _, candidate := range allowlist {
		if strings.EqualFold(requested.Host, candidate.Host) {
			return nil
		}
	}
	msg := "unsupported storage account: %s. This API is configured to work "  +
		"with a pre-defined set of storage accounts. Contact the system admin " +
		"to get your storage account on the allowlist"
	return NewInvalidArgument(fmt.Sprintf(msg, requested.Host))
}
/*
 * Strip leading ? if present from the input SAS token
 */
func sanitizeSAS(sas string) string {
	return strings.TrimPrefix(sas, "?")
}

func splitAzureUrl(path string) (string, string) {
	path = strings.TrimPrefix(path, "/")
	container, blobPath, _ := strings.Cut(path, "/")
	return container, blobPath
}

type ConnectionMaker func(blob, sas string) (Connection, error)

func MakeAzureConnection(accounts []string) ConnectionMaker {
	var allowlist []*url.URL
	for _, account := range accounts {
		account = strings.TrimSpace(account)
		if len(account) == 0 {
			panic("Empty storage-account not allowed")
		}
		url, err := url.Parse(account)
		if err != nil {
			panic(err)
		}

		allowlist = append(allowlist, url)
	}

	return func(blob string, sas string) (Connection, error) {
		blobUrl, err := makeUrl(blob)
		if err != nil {
			return nil, NewInvalidArgument(err.Error())
		}

		if err := isAllowed(allowlist, blobUrl); err != nil {
			return nil, err
		}

		container, blobPath := splitAzureUrl(blobUrl.Path)
		connection := NewAzureConnection(
			blobPath,
			container,
			blobUrl.Host,
			sanitizeSAS(sas),
		)

		/*
		 * OpenVDS (v3.0.3) segfaults on sas-tokens where the srt-field (allowed
		 * resource types) contains 'c' (container) but _not_ 'o' (object/blob).
		 * Such as tokens are valid in a general sense, but does not provide enough
		 * access in order to read a VDS.
		 *
		 * Once this bug is fixed upstream this manual check can go way
		 */
		if err := validateSasSrt(connection); err != nil {
			return nil, err
		}

		return connection, nil
	}
}
