#ifndef IRODS_REST_CPP_ADMIN_API_IMPLEMENTATION_H
#define IRODS_REST_CPP_ADMIN_API_IMPLEMENTATION_H

#include "irods_rest_api_base.h"

#include "generalAdmin.h"
#include "rodsErrorTable.h"

#include "pistache/router.h"

namespace irods::rest
{
    // this is contractually tied directly to the api implementation
    const std::string service_name{"irods_rest_cpp_admin_server"};

    class admin : public api_base
    {
    public:
        admin()
            : api_base{service_name}
        {
            info("Endpoint initialized.");
        }

        std::tuple<Pistache::Http::Code, std::string>
        operator()(const Pistache::Rest::Request& _request,
                   Pistache::Http::ResponseWriter& _response)
        {
            try {
                auto _action = _request.query().get("action").get();
                auto _target = _request.query().get("target").get();
                auto _arg2   = _request.query().get("arg2").get();
                auto _arg3   = _request.query().get("arg3").get();
                auto _arg4   = _request.query().get("arg4").get();
                auto _arg5   = _request.query().get("arg5").get();
                auto _arg6   = _request.query().get("arg6").get();
                auto _arg7   = _request.query().get("arg7").get();

                auto conn = get_connection(_request.headers().getRaw("authorization").value());

                const auto resc_name = decode_url(_arg2);
                const auto vault_path = decode_url(_arg4);
                const auto zone_name = decode_url(_arg6);

                debug("decoded arguments - _arg2=[{}], _arg4=[{}], _arg6=[{}]",
                      resc_name, vault_path, zone_name);

                generalAdminInp_t input{};
                input.arg0 = (_action == "remove") ? "rm" : _action.c_str();
                input.arg1 = _target.c_str();
                input.arg2 = resc_name.c_str();
                input.arg3 = _arg3.c_str();
                input.arg4 = vault_path.c_str();
                input.arg5 = _arg5.c_str();
                input.arg6 = zone_name.c_str();
                input.arg7 = _arg7.c_str();

                trace("Invoking rcGeneralAdmin() ...");

                if (const auto ec = rcGeneralAdmin(conn(), &input); ec < 0) {
                    error("Received error [{}] from rcGeneralAdmin.", ec);
                    return make_error_response(ec, "Error on rcGeneralAdmin.");
                }

                return std::make_tuple(Pistache::Http::Code::Ok, SUCCESS);
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
    }; // class admin
} // namespace irods::rest

#endif // IRODS_REST_CPP_ADMIN_API_IMPLEMENTATION_H

