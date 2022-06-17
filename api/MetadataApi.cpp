#include "MetadataApi.h"

#include "constants.hpp"

#include <spdlog/spdlog.h>

namespace io {
namespace swagger {
namespace server {
namespace api {

using namespace io::swagger::server::model;

MetadataApi::MetadataApi(Pistache::Address addr)
    : httpEndpoint(std::make_shared<Pistache::Http::Endpoint>(addr))
{
}

void MetadataApi::init(size_t thr = 2) {
    auto opts = Pistache::Http::Endpoint::options()
        .threads(thr);
    httpEndpoint->init(opts);
    setupRoutes();
}

void MetadataApi::start() {
    httpEndpoint->setHandler(router.handler());
    httpEndpoint->serve();
}

void MetadataApi::shutdown() {
    httpEndpoint->shutdown();
}

void MetadataApi::setupRoutes() {
    using namespace Pistache::Rest;

    Routes::Post(router, irods::rest::base_url + "/metadata", Routes::bind(&MetadataApi::handler, this));

    // Default handler, called when a route is not found
    router.addCustomHandler(Routes::bind(&MetadataApi::default_handler, this));
}

void MetadataApi::handler(const Pistache::Rest::Request& request,
                       Pistache::Http::ResponseWriter response)
{
    try {
        this->handler_impl(request, response);
    }
    catch (const std::runtime_error& e) {
        response.send(Pistache::Http::Code::Bad_Request, e.what());
    }
}
void MetadataApi::default_handler(const Pistache::Rest::Request& request,
                               Pistache::Http::ResponseWriter response)
{
    response.send(Pistache::Http::Code::Not_Found, "The requested Metadata method does not exist");
}

}
}
}
}

