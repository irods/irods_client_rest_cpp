#ifndef IRODS_REST_CPP_ZONE_REPORT_API_IMPLEMENTATION_H
#define IRODS_REST_CPP_ZONE_REPORT_API_IMPLEMENTATION_H

#include "irods_rest_api_base.h"

#include <irods/zone_report.h>
#include <irods/irods_at_scope_exit.hpp>

#include <pistache/router.h>

namespace irods::rest
{
    // this is contractually tied directly to the api implementation
    const std::string service_name{"irods_rest_cpp_zone_report_server"};

    class zone_report : public api_base
    {
    public:
        zone_report()
            : api_base{service_name}
        {
            info("Endpoint initialized.");
        }

        std::tuple<Pistache::Http::Code, std::string>
        operator()(const Pistache::Rest::Request& _request,
                   Pistache::Http::ResponseWriter& response)
        {
            try {
                auto conn = get_connection(_request.headers().getRaw("authorization").value());

                BytesBuf* bbuf = nullptr;
                at_scope_exit free_bbuf{[&bbuf] { std::free(bbuf); }};

                trace("Invoking rcZoneReport() ...");

                if (const auto ec = rcZoneReport(conn(), &bbuf); ec < 0) {
                    error("Received error [{}] from rcZoneReport.", ec);
                    return make_error_response(ec, rodsErrorName(ec, nullptr));
                }

                trace("No error on call to rcZoneReport().");

                return std::make_tuple(Pistache::Http::Code::Ok,
                                       std::string(static_cast<char*>(bbuf->buf), bbuf->len));
            }
            catch (const irods::exception& e) {
                error("Caught exception - [error_code={}] {}", e.code(), e.what());
                return make_error_response(e.code(), e.what());
            }
            catch (const std::exception& e) {
                error("Caught exception - {}", e.what());
                return make_error_response(SYS_INVALID_INPUT_PARAM, e.what());
            }
        } // operator()
    }; // class zone_report
} // namespace irods::rest

#endif // IRODS_REST_CPP_ZONE_REPORT_API_IMPLEMENTATION_H

