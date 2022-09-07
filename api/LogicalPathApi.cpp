#include "LogicalPathApi.h"

#include "constants.hpp"

#include <spdlog/spdlog.h>

namespace io::swagger::server::api
{
    using namespace io::swagger::server::model;

    LogicalPathApi::LogicalPathApi(Pistache::Address addr)
        : httpEndpoint(std::make_shared<Pistache::Http::Endpoint>(addr))
    {
    }

    void LogicalPathApi::init(size_t thr = 2)
    {
        auto opts = Pistache::Http::Endpoint::options().threads(thr);
        httpEndpoint->init(opts);
        setupRoutes();
    }

    void LogicalPathApi::start()
    {
        httpEndpoint->setHandler(router.handler());
        httpEndpoint->serve();
    }

    void LogicalPathApi::shutdown()
    {
        httpEndpoint->shutdown();
    }

    void LogicalPathApi::setupRoutes()
    {
        using namespace Pistache::Rest;

        Routes::Delete(
            router, irods::rest::base_url + "/logicalpath", Routes::bind(&LogicalPathApi::delete_handler, this));
        Routes::Post(
            router, irods::rest::base_url + "/logicalpath/rename", Routes::bind(&LogicalPathApi::rename_handler, this));
        Routes::Delete(
            router, irods::rest::base_url + "/logicalpath/trim", Routes::bind(&LogicalPathApi::trim_handler, this));
        Routes::Post(
            router,
            irods::rest::base_url + "/logicalpath/repl",
            Routes::bind(&LogicalPathApi::replicate_handler, this));

        // Default handler, called when a route is not found
        router.addCustomHandler(Routes::bind(&LogicalPathApi::default_handler, this));
    }

    void LogicalPathApi::rename_handler(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response)
    {
        try {
            this->rename_handler_impl(request, response);
        }
        catch (const std::runtime_error& e) {
            response.send(Pistache::Http::Code::Bad_Request, e.what());
        }
    }

    void LogicalPathApi::delete_handler(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response)
    {
        try {
            this->delete_handler_impl(request, response);
        }
        catch (const std::runtime_error& e) {
            response.send(Pistache::Http::Code::Bad_Request, e.what());
        }
    }

    void LogicalPathApi::replicate_handler(
        const Pistache::Rest::Request& request,
        Pistache::Http::ResponseWriter response)
    {
        try {
            this->replicate_handler_impl(request, response);
        }
        catch (const std::runtime_error& e) {
            response.send(Pistache::Http::Code::Bad_Request, e.what());
        }
    }

    void LogicalPathApi::trim_handler(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response)
    {
        try {
            this->trim_handler_impl(request, response);
        }
        catch (const std::runtime_error& e) {
            response.send(Pistache::Http::Code::Bad_Request, e.what());
        }
    }
    void LogicalPathApi::default_handler(
        const Pistache::Rest::Request& request,
        Pistache::Http::ResponseWriter response)
    {
        response.send(Pistache::Http::Code::Not_Found, "The requested Logicalpath method does not exist");
    }

} //namespace io::swagger::server::api
