[comment]: <> (This file is used to generate index.html)
[comment]: <> (Generating index.html via VS Code:)
[comment]: <> (Install extension "Markdown Preview Enhanced")
[comment]: <> (Use "Markdown Preview Enhanced" to preview the index.md file.)
[comment]: <> (Generate new file by right click -> "HTML" -> "HTML [cdn hosted]")

# oneseismic API [<span style="color:green">Running</span>]

## Motivation
Volume Data Storage (VDS) is a file format designed to store large multidimensional data sets. Storing data in a single flat file results in a solution where data requests in the first (primary) dimension are very fast, while requests in subsequent dimensions suffer a significant response time penalty.

<a href="https://community.opengroup.org/osdu/platform/domain-data-mgmt-services/seismic/open-vds">Open VDS</a> is an open source library designed for cloud storage, where the data set is partitioned into sub-cubes of fixed size in each dimension. One consequence of this is a normalization of the access time in different dimensions. Open VDS provides the user with an interface for the global cube, and thus the underlying data structure is hidden from the user.

In cases where the requested data set does not span all dimensions there will be a significant overhead in the data retrieved from the cloud storage. Examples here are <i>fence</i> and <i>slice</i> requests. If the VDS is stored using sub-cubes of size 64, then fence and slice requests to Open VDS will retrieve roughly 64 times more data than the user requested. Reducing the sub-cube size will reduce the overhead at the expense of the time Open VDS uses to manage the underlying data structure. Thus, there is a sweet spot balancing the two. 

<a href="https://github.com/equinor/oneseismic-api">oneseismic-api</a> addresses this issue by enabling the user to run the Open VDS instance in Azure or even better in the same data center as the cloud storage. In this way oneseismic-api utilizes the data centers high internal data rate and minimizes the external data traffic.

## API documentation
Swagger documentation is available <a href="swagger/index.html">here</a>. 

## Source code
oneseismic-api is an open project and the code is available <a href="https://github.com/equinor/oneseismic-api">here</a>.


## A simple python example
```python
import requests
import json
from pprint import pprint

# Server url
server = "http://example.com:8080"

# Url to a vds data set in Azure
vds = 'https://StorageAccount.blob.core.windows.net/ContainerName/VdsDataset'

# Sas token
sas = 'sp=rl&st=2023-09-14T07:18:05Z&se=2023-09-15T07:18:05Z&spr=https&sv=2022-11-02&sr=c&sig=signature'

# Request data using post
response = requests.post(f'{server}/metadata',
                            headers = { 'Content-Type' : 'application/json' },
                            data    = json.dumps({'vds' : vds, 'sas' : sas})
                        )    

# Print the response
if response.ok:
    data = response.json()
    pprint(data)
else:
    print(response.text)
```
