#ifndef MetadataApi_H_
#define MetadataApi_H_

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

    class MetadataApi
    {
    public:
        MetadataApi(Pistache::Address addr);
        virtual ~MetadataApi() {};

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

#endif /* MetadataApi_H_ */
