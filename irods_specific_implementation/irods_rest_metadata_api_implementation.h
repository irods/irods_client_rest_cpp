#ifndef IRODS_REST_CPP_METADATA_API_IMPLEMENTATION_H
#define IRODS_REST_CPP_METADATA_API_IMPLEMENTATION_H

#include "irods_rest_api_base.h"
#include "constants.hpp"

#include <irods/atomic_apply_metadata_operations.h>

#include <pistache/router.h>

namespace irods::rest
{
    // this is contractually tied directly to the api implementation
    const std::string service_name{"irods_rest_cpp_metadata_server"};

    class metadata : public api_base
    {
    public:
        metadata()
            : api_base{service_name}
        {
            info("Endpoint initialized.");
        }

        std::tuple<Pistache::Http::Code, std::string>
        operator()(const Pistache::Rest::Request& _request,
                   Pistache::Http::ResponseWriter& _response)
        {
            try {
                auto _cmds = _request.body();
                auto conn  = get_connection(_request.headers().getRaw("authorization").value());

                char * error_string{};

                if ( const auto ec = rc_atomic_apply_metadata_operations(conn(), _cmds.c_str(), &error_string); ec < 0 ){
                    error("Received error [{}] from rc_atomic_apply_metadata_operations", ec);
                    return make_error_response(ec, error_string);
                }

                return std::make_tuple(
                        Pistache::Http::Code::Ok,
                        ""
                );
            }
            catch (const irods::exception& e) {
                error("Caught exception - [error_code={}] {}", e.code(), e.what());
                return make_error_response(e.code(), e.client_display_what());
            }
            catch (const std::exception& e) {
                error("Caught exception - {}", e.what());
                return make_error_response(SYS_INVALID_INPUT_PARAM, e.what());
            }
            catch (...){
                constexpr auto* msg = "Caught unknown exception while processing metadata operation request";
                error(msg);
                return make_error_response(SYS_UNKNOWN_ERROR, msg);
            }
        } // operator()
    }; // class metadata
} // namespace irods::rest

#endif // IRODS_REST_CPP_METADATA_API_IMPLEMENTATION_H

