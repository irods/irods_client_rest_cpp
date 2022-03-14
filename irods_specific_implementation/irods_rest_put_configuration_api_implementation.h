#ifndef IRODS_REST_CPP_PUT_CONFIGURATION_API_IMPLEMENTATION_H
#define IRODS_REST_CPP_PUT_CONFIGURATION_API_IMPLEMENTATION_H

#include "irods_rest_api_base.h"

#include <irods/irods_default_paths.hpp>
#include <irods/query_builder.hpp>

#include <pistache/router.h>

#include <boost/filesystem.hpp>
#include <fmt/format.h>

#include <fstream>

namespace irods::rest
{
    // this is contractually tied directly to the api implementation
    const std::string service_name{"irods_rest_cpp_put_configuration_server"};

    using json = nlohmann::json;

    class put_configuration : public api_base
    {
    public:
        put_configuration() : api_base{service_name}
        {
            info("Endpoint initialized.");
        }

        std::tuple<Pistache::Http::Code, std::string>
        operator()(const Pistache::Rest::Request& _request,
                   Pistache::Http::ResponseWriter& _response)
        {
#if 1
            return std::make_tuple(Pistache::Http::Code::Not_Implemented, "Not supported at this time.");
#else
            try {
                auto _configuration = _request.query().get("cfg").get();

                auto conn = get_connection(_request.headers().getRaw("authorization").value());
                throw_if_user_is_not_rodsadmin(conn);

                const auto dir = get_irods_config_directory();
                const auto cfg = json::parse(decode_url(_configuration));
                auto err = std::string{};

                for (auto&& [k, v] : cfg.items()) {
                    if (!v.contains("file_name") || !v.contains("contents")) {
                        err += fmt::format("Invalid contents for {}", v.dump(4));
                        continue;
                    }

                    const auto fn = fmt::format("{}/{}", dir.c_str(), v.at("file_name").get_ref<const std::string&>());
                    auto os = std::ofstream(fn.c_str());

                    if (!os.is_open()) {
                        err += fmt::format("Failed to open [{}], ", fn);
                        continue;
                    }

                    os << v.at("contents").dump(4);
                } // for key, val

                const auto& http_resp = err.empty() ? Pistache::Http::Code::Ok : Pistache::Http::Code::Bad_Request;

                return std::make_tuple(http_resp, err);
            }
            catch (const irods::exception& e) {
                error("Caught exception - [error_code={}] {}", e.code(), e.what());
                return make_error_response(e.code(), e.client_display_what());
            }
            catch (const std::exception& e) {
                error("Caught exception - {}", e.what());
                return make_error_response(SYS_INVALID_INPUT_PARAM, e.what());
            }
#endif
        } // operator()
    }; // class put_configuration
} // namespace irods::rest

#endif // IRODS_REST_CPP_PUT_CONFIGURATION_API_IMPLEMENTATION_H

