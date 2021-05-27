# iRODS C++ REST Mid-Tier API

## Building this repository

This is a standard CMake project which may be built with either Ninja or Make.

This code base is built with the iRODS toolchain, which uses Clang.  Since this project depends on Pistache, we also need to build Pistache with Clang in order to link against that project.

First clone the iRODS externals repository.
```
git clone https://github.com/irods/externals
git checkout 4-2-stable
```

Install the latest iRODS CMake and Clang packages
```
irods-cmake
irods-externals-clang-runtime
irods-externals-clang
```

Then within the externals repository, build Pistache with
```
make pistache
```

Then install Pistache with
```
cd pistache<VERSION>-0_src/pistache/build/
make install
```

You will then need to install iRODS packages
```
irods-dev
irods-runtime
irods-externals-boost
irods-externals-json
```

Once this is done you can create a build directory for the REST API and run CMake, then run
```
make package
```
to build the REST API package.

## Configuring the service

The REST API provides an executable for each individual API endpoint.  These endpoints may be grouped behind a reverse proxy in order to provide a single port for access.

The services rely on a configuration file in `/etc/irods/` which dictates which ports are used for each service.  Two template files are placed there by the package:
```
/etc/irods/irods_client_rest_cpp.json.template
/etc/irods/irods-client-rest-cpp-reverse-proxy.conf.template
```
`/etc/irods/irods_client_rest_cpp.json.template` should be copied to `/etc/irods/irods_client_rest_cpp.json` and modified if different ports are desired.  The service can then be restarted with `service irods_client_rest_cpp restart`, or however your specific platform manages services.

Once the REST API is running install nginx and then copy `/etc/irods/irods-client-rest-cpp-reverse-proxy.conf.template` to `/etc/nginx/sites-available/irods-client-rest-cpp-reverse-proxy.conf` and then symbolically link it to `/etc/nginx/sites-enabled/irods-client-rest-cpp-reverse-proxy.conf`.   Nginx will then need to be restarted with `sudo service nginx restart`, or however your specific platform manages services.

## Interacting with the API endpoints
The design of this API using JWTs to contain authorization and identity.  The Auth endpoint must be invoked first in order to authenticate and receive a JWT.  This token will then need to be included in the Authorization header of each subsequent request.  This API follows a [HATEOAS](https://en.wikipedia.org/wiki/HATEOAS#:~:text=Hypermedia%20as%20the%20Engine%20of,provide%20information%20dynamically%20through%20hypermedia.) design which provides not only the requested information but possible next operations on that information.

### /access
This endpoint provides a service for the generation of an iRODS ticket to a given logical path, be that a collection or a data object.

**Method** : POST

**Parameters:**
- path: The url encoded logical path to a collection or data object for which access is desired

**Example CURL Command:**
```
curl -X POST -H "Authorization: ${TOKEN}" "http://localhost/irods-rest/1.0.0/access?path=%2FtempZone%2Fhome%2Frods%2Ffile0"
```

**Returns**

An iRODS ticket token within the X-API-KEY header, and a URL for streaming the object.
```
{
  "headers": [
    "X-API-KEY: CS11B8C4KZX2BIl"
  ],
  "url": "/irods-rest/1.0.0/stream?path=%2FtempZone%2Fhome%2Frods%2Ffile0&offset=0&limit=33064"
}
```

### /admin
The administration interface to the iRODS Catalog which allows the creation, removal and modification of users, groups, resources, and other entities within the zone.

**Method** : POST

**Parameters**
- action : dictates the action taken: add, modify, or remove
- target : the subject of the action: user, zone, resource, childtoresc, childfromresc, token, group, rebalance, unusedAVUs, specificQuery
- arg2 : generic argument, could be user name, resource name, depending on the value of `action` and `target`
- arg3 : generic argument , see above
- arg4 : generic argument , see above
- arg5 : generic argument , see above
- arg6 : generic argument , see above
- arg7 : generic argument , see above

**Example CURL Command:**
```
curl -X POST -H "Authorization: ${TOKEN}" "http://localhost/irods-rest/1.0.0/admin?action=add&target=resource&arg2=ufs0&arg3=unixfilesystem&arg4=/tmp/irods/ufs0&arg5=&arg6=tempZone"
```

**Returns**

"Success" or an iRODS exception

### /auth
This endpoint provides an authentication service for the iRODS zone, currently only native iRODS authentication is supported.

**Method** : POST

**Parameters:**
- user_name : the iRODS user which wishes to authenticate
- password : The user's secret, depending on the method of authentication
- auth_type : The iRODS method of authentication - native, pam, krb, gsi, etc.

**Example CURL Command:**
```
export TOKEN=$(curl -X POST "http://localhost:80/irods-rest/1.0.0/auth?user_name=rods&password=rods&auth_type=native")
```

**Returns:**

An encrypted JWT which contains everything necessary to interact with the other endpoints.  This token is expected in the Authorization header for the other services.

### /configuration
This endpoint will return a JSON structure holding the configuration for an iRODS server or write the url encoded JSON to the specified files in `/etc/irods`

**Method** : GET

**Parameters**
- None

**Example CURL Command:**
```
curl -X GET -H "X-API-KEY: ${API_KEY}" "http://localhost/irods-rest/1.0.0/configuration" | jq
```

**Returns**
A json array of objects whose key is the file name and whose contents is the configuration file.
```
{
    "host_access_control_config.json": {
        <SNIP>
    },
    "hosts_config.json": {
        <SNIP>
    },
    "irods_client_rest_cpp.json": {
        <SNIP>
    },
    "server_config.json": {
        <SNIP>
    },
    "server_config.json": {
        <SNIP>
    }
}
```

**Method** : PUT

**Parameters**
- cfg : a url encoded json string of the format
```JSON
[
    {
        "file_name":"test_rest_cfg_put_1.json",
        "contents" : {
            "key0":"value0",
            "key1" : "value1"
        }
    },
    {
        "file_name":"test_rest_cfg_put_2.json",
        "contents" : {
            "key2" : "value2",
            "key3" : "value3"
        }
    }
]
```

**Example CURL Command:**
```
export CONTENTS="%5B%7B%22file_name%22%3A%22test_rest_cfg_put_1.json%22%2C%20%22contents%22%3A%7B%22key0%22%3A%22value0%22%2C%22key1%22%20%3A%20%22value1%22%7D%7D%2C%7B%22file_name%22%3A%22test_rest_cfg_put_2.json%22%2C%22contents%22%3A%7B%22key2%22%20%3A%20%22value2%22%2C%22key3%22%20%3A%20%22value3%22%7D%7D%5D"
curl -X PUT -H "Authorization: ${TOKEN}" "http://localhost/irods-rest/1.0.0/configuration?cfg=${CONTENTS}"
```

**Returns**
Nothing on success

### /list
This endpoint provides a recursive listing of a collection, or stat, metadata, and access control information for a given data object.

**Method** : GET

**Parameters**
- path : The url encoded logical path which is to be listed
- stat : Boolean flag to indicate stat information is desired
- permissions : Boolean flag to indicate access control information is desired
- metadata : Boolean flag to indicate metadata is desired
- offset : number of records to skip for pagination
- limit : number of records desired per page

**Example CURL Command:**
```
curl -X GET -H "Authorization: ${TOKEN}" "http://localhost/irods-rest/1.0.0/list?path=%2FtempZone%2Fhome%2Frods&stat=0&permissions=0&metadata=0&offset=0&limit=100" | jq
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
    "first": "/irods-rest/1.0.0/list?path=%2FtempZone%2Fhome%2Frods&stat=0&permissions=0&metadata=0&offset=0&limit=100",
    "last": "/irods-rest/1.0.0/list?path=%2FtempZone%2Fhome%2Frods&stat=0&permissions=0&metadata=0&offset=UNSUPPORTED&limit=100",
    "next": "/irods-rest/1.0.0/list?path=%2FtempZone%2Fhome%2Frods&stat=0&permissions=0&metadata=0&offset=100&limit=100",
    "prev": "/irods-rest/1.0.0/list?path=%2FtempZone%2Fhome%2Frods&stat=0&permissions=0&metadata=0&offset=0&limit=100",
    "self": "/irods-rest/1.0.0/list?path=%2FtempZone%2Fhome%2Frods&stat=0&permissions=0&metadata=0&offset=0&limit=100"
  }
}
```

### /query
This endpoint provides access to the iRODS General Query language, which is a generic query service for the iRODS catalog.

**Method** : GET

**Parameters**
- query_string : A url encoded general query
- query_limit : Number of desired rows
- row_offset : Number of rows to skip for paging
- query_type : Either 'general' or 'specific'

**Example CURL Command:**
```
curl -X GET -H "Authorization: ${TOKEN}" "http://localhost/irods-rest/1.0.0/query?query_limit=100&row_offset=0&query_type=general&query_string=SELECT%20COLL_NAME%2C%20DATA_NAME%20WHERE%20COLL_NAME%20LIKE%20%27%2FtempZone%2Fhome%2Frods%25%27" | jq
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
    "first": "/irods-rest/1.0.0query?query_string=SELECT%20COLL_NAME%2C%20DATA_NAME%20WHERE%20COLL_NAME%20LIKE%20%27%2FtempZone%2Fhome%2Frods%25%27&query_limit=100&row_offset=0&query_type=general",
    "last": "/irods-rest/1.0.0query?query_string=SELECT%20COLL_NAME%2C%20DATA_NAME%20WHERE%20COLL_NAME%20LIKE%20%27%2FtempZone%2Fhome%2Frods%25%27&query_limit=100&row_offset=0&query_type=general",
    "next": "/irods-rest/1.0.0query?query_string=SELECT%20COLL_NAME%2C%20DATA_NAME%20WHERE%20COLL_NAME%20LIKE%20%27%2FtempZone%2Fhome%2Frods%25%27&query_limit=100&row_offset=0&query_type=general",
    "prev": "/irods-rest/1.0.0query?query_string=SELECT%20COLL_NAME%2C%20DATA_NAME%20WHERE%20COLL_NAME%20LIKE%20%27%2FtempZone%2Fhome%2Frods%25%27&query_limit=100&row_offset=0&query_type=general",
    "self": "/irods-rest/1.0.0query?query_string=SELECT%20COLL_NAME%2C%20DATA_NAME%20WHERE%20COLL_NAME%20LIKE%20%27%2FtempZone%2Fhome%2Frods%25%27&query_limit=100&row_offset=0&query_type=general"
  },
  "count": "4",
  "total": "4"
}
```


### /stream
Stream data into and out of an iRODS data object

**Method** : GET and PUT

**Parameters**
- path : The url encoded logical path to a data object
- offset : The offset in bytes into the data object
- limit : The maximum number of bytes to read

**Returns**

PUT : Nothing, or iRODS Exception

GET : The data requested in the body of the response

**Example CURL Command:**
```
curl -X PUT -H "Authorization: ${TOKEN}" -d"This is some data" "http://localhost/irods-rest/1.0.0/stream?path=%2FtempZone%2Fhome%2Frods%2FfileX&offset=0&limit=1000"
```
or
```
curl -X GET -H "Authorization: ${TOKEN}" "http://localhost/irods-rest/1.0.0/stream?path=%2FtempZone%2Fhome%2Frods%2FfileX&offset=0&limit=1000"
```

### /zone_report
Requests a JSON formatted iRODS Zone report, containing all configuration information for every server in the grid.

**Method** : POST

**Parameters**
- None

**Example CURL Command:**
```
curl -X POST -H "Authorization: ${TOKEN}" "http://localhost/irods-rest/1.0.0/zone_report" | jq
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
