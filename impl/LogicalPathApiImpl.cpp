#include "LogicalPathApiImpl.h"

#include "utils.hpp"

#include <string>

namespace io::swagger::server::api
{
    using namespace io::swagger::server::model;

    LogicalPathApiImpl::LogicalPathApiImpl(Pistache::Address addr)
        : LogicalPathApi(addr)
        , irods_logical_path_{}
    {
    }

    void LogicalPathApiImpl::delete_handler_impl(const Pistache::Rest::Request& request,
                                                 Pistache::Http::ResponseWriter& response)
    {
        const auto irods_logic = [this](const Pistache::Rest::Request& _req, Pistache::Http::ResponseWriter& _res) {
            return irods_logical_path_.delete_dispatcher(_req, _res);
        };
        irods::rest::handle_request(irods_logic, request, response);
    }

    void LogicalPathApiImpl::rename_handler_impl(const Pistache::Rest::Request& request,
                                                 Pistache::Http::ResponseWriter& response)
    {
        const auto irods_logic = [this](const Pistache::Rest::Request& _req, Pistache::Http::ResponseWriter& _res) {
            return irods_logical_path_.rename_dispatcher(_req, _res);
        };
        irods::rest::handle_request(irods_logic, request, response);
    }

    void LogicalPathApiImpl::trim_handler_impl(
        const Pistache::Rest::Request& request,
        Pistache::Http::ResponseWriter& response)
    {
        const auto irods_logic = [this](const Pistache::Rest::Request& _req, Pistache::Http::ResponseWriter& _res) {
            return irods_logical_path_.trim_dispatcher(_req, _res);
        };
        irods::rest::handle_request(irods_logic, request, response);
    }

    void LogicalPathApiImpl::replicate_handler_impl(
        const Pistache::Rest::Request& request,
        Pistache::Http::ResponseWriter& response)
    {
        const auto irods_logic = [this](const Pistache::Rest::Request& _req, Pistache::Http::ResponseWriter& _res) {
            return irods_logical_path_.replicate_dispatcher(_req, _res);
        };
        irods::rest::handle_request(irods_logic, request, response);
    }
} // namespace io::swagger::server::api
