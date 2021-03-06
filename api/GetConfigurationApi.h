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
/*
 * GetConfigurationApi.h
 *
 *
 */

#ifndef GetConfigurationApi_H_
#define GetConfigurationApi_H_


#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/router.h>
#include <pistache/http_headers.h>
#include <pistache/optional.h>
#include "ModelBase.h"

#include <string>

#include "irods_rest_get_configuration_api_implementation.h"

namespace io {
namespace swagger {
namespace server {
namespace api {

using namespace io::swagger::server::model;

class  GetConfigurationApi {
public:
    GetConfigurationApi(Pistache::Address addr);
    virtual ~GetConfigurationApi() {};
    void init(size_t thr);
    void start();
    void shutdown();

    const std::string base = "/irods-rest/1.0.0";

private:
    void setupRoutes();

    void obtain_token_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);
    void get_configuration_api_default_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);

    std::shared_ptr<Pistache::Http::Endpoint> httpEndpoint;
    Pistache::Rest::Router router;

    irods::rest::get_configuration irods_get_configuration_;
};

}
}
}
}

#endif /* GetConfigurationApi_H_ */

