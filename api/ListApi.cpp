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

#include "ListApi.h"

#include "constants.hpp"

#include <spdlog/spdlog.h>

namespace io {
namespace swagger {
namespace server {
namespace api {

using namespace io::swagger::server::model;

ListApi::ListApi(Pistache::Address addr)
    : httpEndpoint(std::make_shared<Pistache::Http::Endpoint>(addr))
{ };

void ListApi::init(size_t thr = 2) {
    auto opts = Pistache::Http::Endpoint::options()
        .threads(thr);
    httpEndpoint->init(opts);
    setupRoutes();
}

void ListApi::start() {
    httpEndpoint->setHandler(router.handler());
    httpEndpoint->serve();
}

void ListApi::shutdown() {
    httpEndpoint->shutdown();
}

void ListApi::setupRoutes() {
    using namespace Pistache::Rest;

    Routes::Get(router, irods::rest::base_url + "/list", Routes::bind(&ListApi::handler, this));

    // Default handler, called when a route is not found
    router.addCustomHandler(Routes::bind(&ListApi::default_handler, this));
}

void ListApi::handler(const Pistache::Rest::Request& request,
                      Pistache::Http::ResponseWriter response)
{
    try {
        this->handler_impl(request, response);
    }
    catch (const std::runtime_error& e) {
        response.send(Pistache::Http::Code::Bad_Request, e.what());
    }
}

void ListApi::default_handler(const Pistache::Rest::Request& request,
                              Pistache::Http::ResponseWriter response)
{
    response.send(Pistache::Http::Code::Not_Found, "The requested List method does not exist");
}

}
}
}
}

