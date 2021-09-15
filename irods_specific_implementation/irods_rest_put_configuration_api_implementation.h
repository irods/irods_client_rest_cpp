#include "irods_rest_api_base.h"

#include "irods_default_paths.hpp"
#include "query_builder.hpp"

#include <boost/filesystem.hpp>
#include <fmt/format.h>

#include <fstream>

// this is contractually tied directly to the pistache implementation, and the below implementation
#define MACRO_IRODS_CONFIGURATION_PUT_API_IMPLEMENTATION \
    auto [code, message] = irods_put_configuration_(request.headers().getRaw("authorization").value(), cfg.get()); \
    response.send(code, message);

namespace irods::rest
{
    using json = nlohmann::json;

    // this is contractually tied directly to the api implementation
    const std::string service_name{"irods_rest_cpp_put_configuration_server"};

    class put_configuration : public api_base
    {
    public:
        put_configuration() : api_base{service_name}
        {
            trace("Endpoint initialized.");
        }

        auto operator()(const std::string& _auth_header, const std::string& _configuration)
            -> std::tuple<Pistache::Http::Code, std::string>
        {
            trace("Handling request ...");

            try {
                auto conn = get_connection(_auth_header);

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
            catch(const irods::exception& _e) {
                auto error = make_error(_e.code(), _e.client_display_what());
                return std::forward_as_tuple(
                           Pistache::Http::Code::Bad_Request,
                           error);
            }
        } // operator()
    }; // class put_configuration
} // namespace irods::rest

