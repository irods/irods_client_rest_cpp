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

#include "ZoneReportApi.h"

namespace io {
namespace swagger {
namespace server {
namespace api {

using namespace io::swagger::server::model;

ZoneReportApi::ZoneReportApi(Pistache::Address addr)
    : httpEndpoint(std::make_shared<Pistache::Http::Endpoint>(addr))
{ };

void ZoneReportApi::init(size_t thr = 2) {
    auto opts = Pistache::Http::Endpoint::options()
        .threads(thr);
    httpEndpoint->init(opts);
    setupRoutes();
}

void ZoneReportApi::start() {
    httpEndpoint->setHandler(router.handler());
    httpEndpoint->serve();
}

void ZoneReportApi::shutdown() {
    httpEndpoint->shutdown();
}

void ZoneReportApi::setupRoutes() {
    using namespace Pistache::Rest;

    Routes::Post(router, base + "/zone_report", Routes::bind(&ZoneReportApi::obtain_token_handler, this));

    // Default handler, called when a route is not found
    router.addCustomHandler(Routes::bind(&ZoneReportApi::zone_report_api_default_handler, this));
}

void ZoneReportApi::obtain_token_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response) {
    try {
      this->obtain_token(request.headers(), request.body(), response);
    } catch (std::runtime_error & e) {
      //send a 400 error
      response.send(Pistache::Http::Code::Bad_Request, e.what());
      return;
    }
}

void ZoneReportApi::zone_report_api_default_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response) {
    response.send(Pistache::Http::Code::Not_Found, "The requested zone_report method does not exist");
}

}
}
}
}

