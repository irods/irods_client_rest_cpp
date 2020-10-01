/**
* iRODS REST API
* This is the iRODS REST API
*
* OpenAPI spec version: 1.0.0
* Contact: info@irods.org
*
* NOTE: This class is auto generated by the swagger code generator program.
* https://github.com/swagger-api/swagger-codegen.git
* Do not edit the class manually.
*/

#include "pistache/endpoint.h"
#include "pistache/http.h"
#include "pistache/router.h"
#include "AdminApiImpl.h"

using namespace io::swagger::server::api;

int main() {
    irods::rest::configuration cfg{"irods_rest_cpp_admin_server"};
    int port = cfg["port"];
    Pistache::Address addr(Pistache::Ipv4::any(), Pistache::Port(port));

    AdminApiImpl server(addr);
    server.init(cfg["threads"]);
    server.start();

    server.shutdown();
}

