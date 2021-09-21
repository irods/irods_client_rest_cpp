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
 * ListApi.h
 *
 * 
 */

#ifndef ListApi_H_
#define ListApi_H_


#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/router.h>
#include <pistache/http_headers.h>
#include <pistache/optional.h>

#include "ModelBase.h"

#include <string>

namespace io {
namespace swagger {
namespace server {
namespace api {

using namespace io::swagger::server::model;

class  ListApi {
public:
    ListApi(Pistache::Address addr);
    virtual ~ListApi() {};

    void init(size_t thr);
    void start();
    void shutdown();

private:
    void setupRoutes();

    void handler(const Pistache::Rest::Request& request,
                 Pistache::Http::ResponseWriter response);

    void default_handler(const Pistache::Rest::Request& request,
                         Pistache::Http::ResponseWriter response);

    virtual void handler_impl(const Pistache::Rest::Request& request,
                              Pistache::Http::ResponseWriter& response) = 0;

    std::shared_ptr<Pistache::Http::Endpoint> httpEndpoint;
    Pistache::Rest::Router router;
};

}
}
}
}

#endif /* ListApi_H_ */

