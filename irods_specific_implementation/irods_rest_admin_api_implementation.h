#ifndef IRODS_REST_CPP_ADMIN_API_IMPLEMENTATION_H
#define IRODS_REST_CPP_ADMIN_API_IMPLEMENTATION_H

#include "irods_rest_api_base.h"

#include <irods/generalAdmin.h>
#include <irods/rodsErrorTable.h>
#include <irods/obf.h>
#include <irods/authenticate.h>

#include <pistache/router.h>

#include <cstring>
#include <array>
#include <string>
#include <string_view>

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

                const auto decoded_arg2 = decode_url(_arg2);
                const auto decoded_arg4 = decode_url(_arg4);
                const auto decoded_arg6 = decode_url(_arg6);

                generalAdminInp_t input{};
                std::string obfuscated_password;

                if (_action == "modify" && _target == "user" && _arg3 == "password") {
                    // DO NOT print the user's password.
                    debug("decoded arguments - _arg2=[{}], _arg6=[{}]", decoded_arg2, decoded_arg6);

                    input.arg0 = _action.c_str();
                    obfuscated_password = obfuscate_password(decoded_arg4);
                    input.arg4 = obfuscated_password.c_str();
                }
                else {
                    debug("decoded arguments - _arg2=[{}], _arg4=[{}], _arg6=[{}]",
                          decoded_arg2, decoded_arg4, decoded_arg6);

                    input.arg0 = (_action == "remove") ? "rm" : _action.c_str();
                    input.arg4 = decoded_arg4.c_str();
                }

                input.arg1 = _target.c_str();
                input.arg2 = decoded_arg2.c_str();
                input.arg3 = _arg3.c_str();
                input.arg5 = _arg5.c_str();
                input.arg6 = decoded_arg6.c_str();
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

        std::string obfuscate_password(const std::string_view _new_password)
        {
            std::array<char, MAX_PASSWORD_LEN + 10> plain_text_password{};
            std::strncpy(plain_text_password.data(), _new_password.data(), MAX_PASSWORD_LEN);

            if (const auto lcopy = MAX_PASSWORD_LEN - 10 - _new_password.size(); lcopy > 15) {
                // The random string (second argument) is used for padding and must match 
                // what is defined on the server-side.
                std::strncat(plain_text_password.data(), "1gCBizHWbwIYyWLoysGzTe6SyzqFKMniZX05faZHWAwQKXf6Fs", lcopy);
            }

            std::array<char, MAX_PASSWORD_LEN + 10> admin_password{};

            // Get the plain text password of the iRODS service account user.
            // "obfGetPw" decodes the obfuscated password stored in .irods/.irodsA.
            if (obfGetPw(admin_password.data()) != 0) {
                THROW(SYS_INTERNAL_ERR, "failed to unobfuscate admin password in .irodsA file.");
            }

            std::array<char, MAX_PASSWORD_LEN + 100> obfuscated_password{};
            obfEncodeByKeyV2(plain_text_password.data(),
                             admin_password.data(),
                             getSessionSignatureClientside(),
                             obfuscated_password.data());

            return obfuscated_password.data();
        } // obfuscate_password
    }; // class admin
} // namespace irods::rest

#endif // IRODS_REST_CPP_ADMIN_API_IMPLEMENTATION_H

