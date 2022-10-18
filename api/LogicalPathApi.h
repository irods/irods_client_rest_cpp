#ifndef LogicalPathApi_H_
#define LogicalPathApi_H_

#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/router.h>
#include <pistache/http_headers.h>
#include <pistache/optional.h>

#include "ModelBase.h"

#include <string>

namespace io::swagger::server::api
{
    using namespace io::swagger::server::model;

    class LogicalPathApi
    {
    public:
        LogicalPathApi(Pistache::Address addr);
        virtual ~LogicalPathApi(){};

        void init(size_t thr);
        void start();
        void shutdown();

    private:
        void setupRoutes();

        void default_handler(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);

        void post_handler(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
        void delete_handler(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
        void rename_handler(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
        void trim_handler(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
        void replicate_handler(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);

        virtual void post_handler_impl(
            const Pistache::Rest::Request& request,
            Pistache::Http::ResponseWriter& response) = 0;
        virtual void delete_handler_impl(
            const Pistache::Rest::Request& request,
            Pistache::Http::ResponseWriter& response) = 0;
        virtual void rename_handler_impl(
            const Pistache::Rest::Request& request,
            Pistache::Http::ResponseWriter& response) = 0;
        virtual void trim_handler_impl(
            const Pistache::Rest::Request& request,
            Pistache::Http::ResponseWriter& response) = 0;
        virtual void replicate_handler_impl(
            const Pistache::Rest::Request& request,
            Pistache::Http::ResponseWriter& response) = 0;

        std::shared_ptr<Pistache::Http::Endpoint> httpEndpoint;
        Pistache::Rest::Router router;
    };
} // namespace io::swagger::server::api

#endif /* LogicalPathApi_H_ */
