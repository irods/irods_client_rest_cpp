#include "irods_rest_api_base.h"

#include "zone_report.h"
#include "irods_at_scope_exit.hpp"

// this is contractually tied directly to the pistache implementation, and the below implementation
#define MACRO_IRODS_ZONE_REPORT_API_IMPLEMENTATION \
    Pistache::Http::Code code; \
    std::string message; \
    std::tie(code, message) = irods_zone_report_(headers.getRaw("authorization").value()); \
    response.send(code, message);

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
            trace("Endpoint initialized.");
        }

        std::tuple<Pistache::Http::Code, std::string>
        operator()(const std::string& _auth_header)
        {
            trace("Handling request ...");

            try {
                auto conn = get_connection(_auth_header);

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

