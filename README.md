# iRODS C++ REST Mid-Tier API

This REST API is designed to be deployed in front of an iRODS Server to provide an HTTP REST interface into the iRODS protocol.

This REST API requires an iRODS Server v4.2.0 or greater.

## Quickstart

The iRODS C++ REST API can be installed via package manager and managed via `systemctl` alongside an existing iRODS Server:

```
# REST API - install
$ sudo apt install irods-client-rest-cpp
OR
$ sudo yum install irods-client-rest-cpp
```

nginx, rsyslog, and logrotate can be configured and restarted before starting the REST API:

```
# nginx - configure and restart
$ sudo cp /etc/irods_client_rest_cpp/irods_client_rest_cpp_reverse_proxy.conf.template /etc/nginx/sites-available/irods_client_rest_cpp_reverse_proxy.conf
$ sudo ln -s /etc/nginx/sites-available/irods_client_rest_cpp_reverse_proxy.conf /etc/nginx/sites-enabled/irods_client_rest_cpp_reverse_proxy.conf
$ sudo systemctl restart nginx

# rsyslog - configure and restart
$ sudo cp /etc/irods_client_rest_cpp/irods_client_rest_cpp.conf.rsyslog /etc/rsyslog.d/00-irods_client_rest_cpp.conf
$ sudo cp /etc/irods_client_rest_cpp/irods_client_rest_cpp.logrotate /etc/logrotate.d/irods_client_rest_cpp
$ sudo systemctl restart rsyslog

# REST API - configure and start
$ sudo cp /etc/irods_client_rest_cpp/irods_client_rest_cpp.json.template /etc/irods_client_rest_cpp/irods_client_rest_cpp.json
$ sudo systemctl restart irods_client_rest_cpp
```

## Building this repository
This is a standard CMake project which may be built with either Ninja or Make.

This project depends on `irods-dev(el)` and `irods-runtime` version 4.3.0 (available from https://packages.irods.org). The minimum required CMake is 3.12.0. You can install the latest `irods-externals-cmake` package if needed (also available from https://packages.irods.org).

Once these are installed, the other `irods-externals-*` versions will be revealed through building with `cmake` and may change over time as this code base develops.

Prerequisites ():
```
irods-dev(el)
irods-runtime
irods-externals-boost
irods-externals-clang
irods-externals-json
irods-externals-pistache
irods-externals-spdlog
```

Create and enter a build directory, run CMake (make sure it is visible in your `$PATH`), and then make the REST API package:
```
$ mkdir build && cd build && cmake ..
$ make -j package
```

## Building with Docker

This repository provides a Dockerfile with all of the tools needed to build a package for Ubuntu 20.04.

The builder image itself must first be built. This can be done by running the following:
```bash
cd docker/builder
docker build -t irods-client-rest-cpp-builder:ubuntu-20.04 .
```

This will produce a Docker Image with tag `irods-client-rest-cpp-builder:ubuntu-20.04`. The tag can be anything, as long as it is used consistently.

Once built, the builder can be run to build packages from source code on the host machine. The script run by the container takes the following inputs:

1. A volume mount for the source directory (this repository) on the host
2. A volume mount for the output build files on the host
3. A volume mount for the package by itself on the host

Here is an example usage:
```bash
docker run -t --rm \
           -v ${sourcedir}:/src:ro \
           -v ${builddir}:/build \
           -v ${packagedir}:/packages \
           irods-client-rest-cpp-builder:ubuntu-20.04
```
`$sourcedir`, `$builddir`, and `$packagedir` should be full paths to the source code directory, the desired output directory for build artifacts, and the desired output directory for the built package.

Once the build has completed, you should find a `.deb` file on the host machine where you specified `$packagedir` to be.

Integrating this into your workflow is an exercise left to the reader.

## Configuration files
The REST API provides an executable for each individual API endpoint. These endpoints may be grouped behind a reverse proxy in order to provide a single port for access.

The service relies on a configuration file in `/etc/irods_client_rest_cpp` which dictates which ports are used. Two template files are placed there by the package:
```bash
/etc/irods_client_rest_cpp/irods_client_rest_cpp.json.template
/etc/irods_client_rest_cpp/irods_client_rest_cpp_reverse_proxy.conf.template
```

## Starting the service
To start the REST API service, run the following commands:
```bash
$ sudo cp /etc/irods_client_rest_cpp/irods_client_rest_cpp.json.template /etc/irods_client_rest_cpp/irods_client_rest_cpp.json # configuration
$ sudo systemctl start irods_client_rest_cpp # start the service
```

If everything was successful, you now have a functional REST API service.

If you modify the configuration file (i.e. port numbers, log level, etc.). You'll need to restart the service for the changes to take affect.

## Starting the reverse proxy using Nginx
This section assumes you have a functional REST API service.

The first thing you must do is install `nginx`. Once installed, run the following commands to enable the reverse proxy:
```bash
$ sudo cp /etc/irods_client_rest_cpp/irods_client_rest_cpp_reverse_proxy.conf.template /etc/nginx/sites-available/irods_client_rest_cpp_reverse_proxy.conf # configuration
$ sudo ln -s /etc/nginx/sites-available/irods_client_rest_cpp_reverse_proxy.conf /etc/nginx/sites-enabled/irods_client_rest_cpp_reverse_proxy.conf # configuration
$ sudo systemctl restart nginx # start the service
```

If you modified any port numbers in the REST API's configuration file, you will need to adjust the reverse proxy's configuration file so that `nginx` can connect to the correct endpoint.

If you are getting a 404 error from the reverse proxy, make sure to remove the `default` file from `/etc/nginx/sites-enabled` directory. This default site may be listening on the same port as your reverse proxy and preventing the requests from reaching it.

## Enabling logging via Rsyslog and Logrotate
_This section assumes you've installed the C++ REST API package._

_If you are doing a non-package install, you'll need to copy the configuration files from the repository to the appropriate directories and restart rsyslog. The files of interest and their destinations are shown below._

The REST API uses rsyslog and logrotate for log file management. To enable, run the following commands:
```bash
$ sudo cp /etc/irods_client_rest_cpp/irods_client_rest_cpp.conf.rsyslog /etc/rsyslog.d/00-irods_client_rest_cpp.conf
$ sudo cp /etc/irods_client_rest_cpp/irods_client_rest_cpp.logrotate /etc/logrotate.d/irods_client_rest_cpp
$ sudo systemctl restart rsyslog
```

The log file will be located at `/var/log/irods_client_rest_cpp/irods_client_rest_cpp.log`.

The log level for each endpoint can be adjusted by modifying the `"log_level"` option in `/etc/irods_client_rest_cpp/irods_client_rest_cpp.json`. The following values are supported:
- trace
- debug
- info
- warn
- error
- critical

## HTTP and SSL
It is highly advised to run the service with HTTPS enabled. The administrative API endpoint (i.e. _/admin_) is implemented to accept passwords in **plaintext**. This is on purpose as it removes the password obfuscation requirements from applications built upon the C++ REST API.

Please refer to your proxy server's (nginx, apache httpd, etc.) documentation for enabling SSL communication.

## Interacting with the API endpoints
The design of this API uses JWTs to contain authorization and identity. The Auth endpoint must be invoked first in order to authenticate and receive a JWT. This token will then need to be included in the Authorization header of each subsequent request. This API follows a [HATEOAS](https://en.wikipedia.org/wiki/HATEOAS#:~:text=Hypermedia%20as%20the%20Engine%20of,provide%20information%20dynamically%20through%20hypermedia.) design which provides not only the requested information but possible next operations on that information.

## Error messages
Unless otherwise specified, successful operations by this client should return with an empty body. If some error is reported, it will follow this format:
```json
{
	"error_code": integer,
	"error_message": string
}
```

### /access
This endpoint provides a service for the generation of a read-only iRODS ticket to a given logical path, be that a collection or a data object.

**Method**: POST

**Parameters:**
- path: The url encoded logical path to a collection or data object for which access is desired
- type: The type of ticket to create. The value must be either read or write. Defaults to read
- use_count: The maximum number of times the ticket can be used. Defaults to 0 (unlimited use)
- write_file_count: The maximum number of writes allowed to a data object. Defaults to 0 (unlimited writes)
- write_byte_count: The maximum number of bytes allowed to be written to data object. Defaults to 0 (unlimited bytes)
- seconds_until_expiration: The number of seconds before the ticket will expire. Defaults to 0 (no expiration)
- users: A comma-separated list of iRODS users who are allowed to use the generated ticket
- groups: A comma-separated list of iRODS groups that are allowed to use the generated ticket
- hosts: A comma-separated list of hosts that are allowed to use the ticket

**Example CURL Command:**
```
curl -X POST -H "Authorization: ${TOKEN}" 'http://localhost/irods-rest/0.9.2/access?path=%2FtempZone%2Fhome%2Frods%2Ffile0&type=write&write_file_count=10'
```

**Returns**

An iRODS ticket token within the **irods-ticket** header, and a URL for streaming the object.
```
{
  "headers": {
    "irods-ticket": ["CS11B8C4KZX2BIl"]
  },
  "url": "/irods-rest/0.9.2/stream?path=%2FtempZone%2Fhome%2Frods%2Ffile0&offset=0&count=33064"
}
```

### /admin
The administration interface to the iRODS Catalog which allows the creation, removal and modification of users, groups, resources, and other entities within the zone. **ALL input parameters must be defined. If an input parameter is not to be used, set it to nothing (e.g. arg7=)**

**Method**: POST

**Parameters**
- action: dictates the action taken: add, modify, or remove
- target: the subject of the action: user, zone, resource, childtoresc, childfromresc, token, group, rebalance, unusedAVUs, specificQuery
- arg2: generic argument, could be user name, resource name, depending on the value of `action` and `target`
- arg3: generic argument, see above
- arg4: generic argument, see above
- arg5: generic argument, see above
- arg6: generic argument, see above
- arg7: generic argument, see above

**Example CURL Command:**
```
curl -X POST -H "Authorization: ${TOKEN}" 'http://localhost/irods-rest/0.9.2/admin?action=add&target=resource&arg2=ufs0&arg3=unixfilesystem&arg4=/tmp/irods/ufs0&arg5=&arg6=tempZone&arg7='
```

**Returns**

"Success" or an iRODS exception

### /auth
This endpoint provides an authentication service for the iRODS zone, currently only native iRODS authentication is supported.

**Method**: POST

**Parameters:**
- Authorization Header of the form `Authorization: Basic ${SECRETS}` where ${SECRETS} is a base 64 encoded string of user_name:password

**Example CURL Command:**
```
export SECRETS=$(echo -n rods:rods | base64)
export TOKEN=$(curl -X POST -H "Authorization: Basic ${SECRETS}" http://localhost:80/irods-rest/0.9.2/auth)
```

**Returns:**

An encrypted JWT which contains everything necessary to interact with the other endpoints. This token is expected in the Authorization header for the other services.

### /get_configuration
This endpoint will return a JSON structure holding the configuration for an iRODS server

**Method**: GET

**Parameters**
- None

**Example CURL Command:**
```
curl -X GET -H "Authorization: ${TOKEN}" 'http://localhost/irods-rest/0.9.2/get_configuration' | jq
```

### /list
This endpoint provides a recursive listing of a collection, or stat, metadata, and access control information for a given data object.

**Method**: GET

**Parameters**
- path: The url encoded logical path which is to be listed
- stat: Boolean flag to indicate stat information is desired
- permissions: Boolean flag to indicate access control information is desired
- metadata: Boolean flag to indicate metadata is desired
- offset: number of records to skip for pagination
- limit: number of records desired per page

**Example CURL Command:**
```
curl -X GET -H "Authorization: ${TOKEN}" 'http://localhost/irods-rest/0.9.2/list?path=%2FtempZone%2Fhome%2Frods&stat=0&permissions=0&metadata=0&offset=0&limit=100' | jq
```

**Returns**

A JSON structured response within the body containing the listing, or an iRODS exception
```
{
  "_embedded": [
    {
      "logical_path": "/tempZone/home/rods/subcoll",
      "type": "collection"
    },
    {
      "logical_path": "/tempZone/home/rods/subcoll/file0",
      "type": "data_object"
    },
    {
      "logical_path": "/tempZone/home/rods/subcoll/file1",
      "type": "data_object"
    },
    {
      "logical_path": "/tempZone/home/rods/subcoll/file2",
      "type": "data_object"
    },
    {
      "logical_path": "/tempZone/home/rods/file0",
      "type": "data_object"
    }
  ],
  "_links": {
    "first": "/irods-rest/0.9.2/list?path=%2FtempZone%2Fhome%2Frods&stat=0&permissions=0&metadata=0&offset=0&limit=100",
    "last": "/irods-rest/0.9.2/list?path=%2FtempZone%2Fhome%2Frods&stat=0&permissions=0&metadata=0&offset=UNSUPPORTED&limit=100",
    "next": "/irods-rest/0.9.2/list?path=%2FtempZone%2Fhome%2Frods&stat=0&permissions=0&metadata=0&offset=100&limit=100",
    "prev": "/irods-rest/0.9.2/list?path=%2FtempZone%2Fhome%2Frods&stat=0&permissions=0&metadata=0&offset=0&limit=100",
    "self": "/irods-rest/0.9.2/list?path=%2FtempZone%2Fhome%2Frods&stat=0&permissions=0&metadata=0&offset=0&limit=100"
  }
}
```

### /logicalpath
Deletes a data object or a collection.

**Method** DELETE

**Parameters**
- logical-path: The absolute path leading to the data object or collection to be deleted.
- no-trash: Don't send to trash, delete permanently. Optional, defaults to false.
- recursive: Recursively delete contents of a collection. Optional, defaults to false.
- unregister: Unregister data objects instead of deleting them. Optional, defaults to false.

**Example CURL command**
```
curl -X DELETE -H "Authorization: ${TOKEN}" "http://localhost:80/irods-rest/0.9.2/logicalpath?logical-path=/tempZone/home/rods/hello.cpp&no-trash=1"
```

**Returns**
Nothing on success.

### /logicalpath/rename
Renames a data object or collection

**Method**: POST

**Parameters**
- src: The path to the data object or collection to be renamed.
- dst: New name of the target data object or collection.

**Example CURL command**
```
curl -X DELETE -H "Authorization: ${TOKEN}" "http://localhost:80/irods-rest/0.9.2/logicalpath/rename?src=/tempZone/home/rods/hello&dst=/tempZone/home/rods/goodbye"
```

**Returns**
Nothing on success.

### /put_configuration
This endpoint will write the url encoded JSON to the specified files in `/etc/irods`

**Method**: PUT

**Parameters**
- cfg: a url encoded json string of the format
```JSON
[
    {
        "file_name":"test_rest_cfg_put_1.json",
        "contents": {
            "key0":"value0",
            "key1": "value1"
        }
    },
    {
        "file_name":"test_rest_cfg_put_2.json",
        "contents": {
            "key2": "value2",
            "key3": "value3"
        }
    }
]
```

**Example CURL Command:**
```
export CONTENTS="%5B%7B%22file_name%22%3A%22test_rest_cfg_put_1.json%22%2C%20%22contents%22%3A%7B%22key0%22%3A%22value0%22%2C%22key1%22%20%3A%20%22value1%22%7D%7D%2C%7B%22file_name%22%3A%22test_rest_cfg_put_2.json%22%2C%22contents%22%3A%7B%22key2%22%20%3A%20%22value2%22%2C%22key3%22%20%3A%20%22value3%22%7D%7D%5D"
curl -X PUT -H "Authorization: ${TOKEN}" "http://localhost/irods-rest/0.9.2/put_configuration?cfg=${CONTENTS}"
```

**Returns**
Nothing on success

### /metadata
This endpoint allows executing multiple metadata operations on a single object atomically.

**Method**: POST

**Parameters**
cmds: A JSON-object encoding a single target, its entity type, and a list of operations to perform on it.
See [here](https://docs.irods.org/4.3.0/doxygen/atomic__apply__metadata__operations_8h.html) for the format of these commands.

**Example CURL Command:**
```
export CMDS="{\
  "entity_name": "foo.cpp",\
  "entity_type": "data_object",\
  "operations": [\
    {\
      "operation": "add",\
      "attribute": "data_object_attrib_foo.cp",\
      "value": "data_object_value_foo.cp",\
      "units": "data_object_units_foo.cp"\
    },\
    {\
      "operation": "remove",\
      "attribute": "data_object_attrib_foo.cp",\
      "value": "data_object_value_foo.cp",\
      "units": "data_object_units_foo.cp"\
    }\
  ]\
}"
curl -X POST -H "Authorization: ${TOKEN}" "http://localhost/irods-rest/0.9.2/metadata?cmds=${CMDS}"
```

**Returns**
Nothing on success

### /query
This endpoint provides access to the iRODS General Query language, which is a generic query service for the iRODS catalog.

**Method**: GET

**Parameters**
- query_string: A url encoded GenQuery string
- query_limit: The max number of rows to return
- row_offset: Number of rows to skip for paging
- query_type: Either 'general' or 'specific'
- case_sensitive: Affects string matching in GenQuery. Defaults to 1
- distinct: Requests distinct rows from GenQuery. Defaults to 1

**Example CURL Command:**
```
curl -X GET -H "Authorization: ${TOKEN}" 'http://localhost/irods-rest/0.9.2/query?query_limit=100&row_offset=0&query_type=general&query_string=SELECT%20COLL_NAME%2C%20DATA_NAME%20WHERE%20COLL_NAME%20LIKE%20%27%2FtempZone%2Fhome%2Frods%25%27' | jq
```

**Returns**
A JSON structure containing the query results
```
{
  "_embedded": [
    [
      "/tempZone/home/rods",
      "file0"
    ],
    [
      "/tempZone/home/rods/subcoll",
      "file0"
    ],
    [
      "/tempZone/home/rods/subcoll",
      "file1"
    ],
    [
      "/tempZone/home/rods/subcoll",
      "file2"
    ]
  ],
  "_links": {
    "first": "/irods-rest/0.9.2/query?query_string=SELECT%20COLL_NAME%2C%20DATA_NAME%20WHERE%20COLL_NAME%20LIKE%20%27%2FtempZone%2Fhome%2Frods%25%27&query_limit=100&row_offset=0&query_type=general&case_sensitive=1&distinct=1",
    "last": "/irods-rest/0.9.2/query?query_string=SELECT%20COLL_NAME%2C%20DATA_NAME%20WHERE%20COLL_NAME%20LIKE%20%27%2FtempZone%2Fhome%2Frods%25%27&query_limit=100&row_offset=0&query_type=general&case_sensitive=1&distinct=1",
    "next": "/irods-rest/0.9.2/query?query_string=SELECT%20COLL_NAME%2C%20DATA_NAME%20WHERE%20COLL_NAME%20LIKE%20%27%2FtempZone%2Fhome%2Frods%25%27&query_limit=100&row_offset=0&query_type=general&case_sensitive=1&distinct=1",
    "prev": "/irods-rest/0.9.2/query?query_string=SELECT%20COLL_NAME%2C%20DATA_NAME%20WHERE%20COLL_NAME%20LIKE%20%27%2FtempZone%2Fhome%2Frods%25%27&query_limit=100&row_offset=0&query_type=general&case_sensitive=1&distinct=1",
    "self": "/irods-rest/0.9.2/query?query_string=SELECT%20COLL_NAME%2C%20DATA_NAME%20WHERE%20COLL_NAME%20LIKE%20%27%2FtempZone%2Fhome%2Frods%25%27&query_limit=100&row_offset=0&query_type=general&case_sensitive=1&distinct=1"
  },
  "count": "4",
  "total": "4"
}
```


### /stream
Stream data into and out of an iRODS data object

**Method**: GET and PUT

**Parameters**
- path: The url encoded logical path to a data object
- offset: The offset in bytes into the data object (Defaults to 0)
- count: The maximum number of bytes to read or write.
  - Required for GET requests.
  - On a GET, this parameter is limited to signed 32-bit integer.
  - On a PUT, this parameter is limited to signed 64-bit integer.
- truncate: Truncates the data object on open
  - Defaults to "true".
  - Applies to PUT requests only.

**Returns**

PUT: Nothing, or iRODS Exception

GET: The data requested in the body of the response

**Example CURL Command:**
```
curl -X PUT -H "Authorization: ${TOKEN}" [-H "irods-ticket: ${TICKET}"] -d"This is some data" 'http://localhost/irods-rest/0.9.2/stream?path=%2FtempZone%2Fhome%2Frods%2FfileX&offset=10'
```
or
```
curl -X GET -H "Authorization: ${TOKEN}" [-H "irods-ticket: ${TICKET}"] 'http://localhost/irods-rest/0.9.2/stream?path=%2FtempZone%2Fhome%2Frods%2FfileX&offset=0&count=1000'
```

### /zone_report
Requests a JSON formatted iRODS Zone report, containing all configuration information for every server in the grid.

**Method**: POST

**Parameters**
- None

**Example CURL Command:**
```
curl -X POST -H "Authorization: ${TOKEN}" 'http://localhost/irods-rest/0.9.2/zone_report' | jq
```

**Returns**
JSON formatted Zone Report

```
{
  "schema_version": "file:///var/lib/irods/configuration_schemas/v3/zone_bundle.json",
  "zones": [
    {
    <snip>
    }]
}
```
