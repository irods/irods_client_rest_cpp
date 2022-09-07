#ifndef LOGICAL_PATH_API_IMPL_H_
#define LOGICAL_PATH_API_IMPL_H_

#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/router.h>
#include <pistache/optional.h>

#include "ModelBase.h"

#include <LogicalPathApi.h>

#include <string>

#include "irods_rest_logical_path_api_implementation.h"

namespace io::swagger::server::api
{
    using namespace io::swagger::server::model;

    class LogicalPathApiImpl : public io::swagger::server::api::LogicalPathApi
    {
    public:
        LogicalPathApiImpl(Pistache::Address addr);

        ~LogicalPathApiImpl() = default;

        void rename_handler_impl(const Pistache::Rest::Request& request,
                                 Pistache::Http::ResponseWriter& response) override;

        void delete_handler_impl(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter& response)
            override;

        void trim_handler_impl(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter& response)
            override;

        void replicate_handler_impl(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter& response)
            override;

        irods::rest::logical_path irods_logical_path_;
    }; // class LogicalPathApiImpl
} // namespace io::swagger::server::api

#endif // LOGICAL_PATH_API_IMPL_H_
