#include "irods_rest_api_base.h"

#include "irods_default_paths.hpp"
#include "query_builder.hpp"

#include <boost/filesystem.hpp>

#include <fstream>

// this is contractually tied directly to the pistache implementation, and the below implementation
#define MACRO_IRODS_CONFIGURATION_GET_API_IMPLEMENTATION \
    Pistache::Http::Code code; \
    std::string message; \
    std::tie(code, message) = irods_get_configuration_(request.headers().getRaw("authorization").value()); \
    response.send(code, message);

namespace irods::rest
{
    namespace bfs = boost::filesystem;

    using json = nlohmann::json;

    // this is contractually tied directly to the api implementation
    const std::string service_name{"irods_rest_cpp_get_configuration_server"};

    class get_configuration : public api_base
    {
        auto ends_with_dot_json(const bfs::directory_entry& _e) -> bool
        {
            const auto s = std::string{".json"};
            const auto p = _e.path().string();
            const auto l = p.length();
            const auto x = p.substr(l - s.length());

            return x == s;
        }

    public:
        get_configuration()
            : api_base{service_name}
        {
            logger_->trace("Endpoint [{}] initialized.", service_name);
        }

        auto operator()(const std::string& _auth_header)
            -> std::tuple<Pistache::Http::Code, std::string>
        {

            try {
                auto conn = get_connection(_auth_header);
                throw_if_user_is_not_rodsadmin(conn);

                const auto dir = get_irods_config_directory();
                auto cfg = json::array();

                for (auto&& e : bfs::recursive_directory_iterator(dir)) {
                    if (!bfs::is_regular_file(e)) {
                        continue;
                    }

                    if (!ends_with_dot_json(e)) {
                        continue;
                    }

                    try {
                        auto f = json::parse(std::ifstream(e.path().c_str()));
                        cfg.push_back(json{e.path().c_str(), "parsing"});
                        cfg.push_back(json{e.path().filename().c_str(), f});
                    }
                    catch (const json::exception& _e) {
                        cfg.push_back(json{e.path().filename().c_str(), _e.what()});
                    }
                } // for e

                return std::make_tuple(Pistache::Http::Code::Ok, cfg.dump());
            }
            catch(const irods::exception& _e) {
                auto error = make_error(_e.code(), _e.client_display_what());
                return std::forward_as_tuple(
                           Pistache::Http::Code::Bad_Request,
                           error);
            }
        } // operator()
    }; // class get_configuration
} // namespace irods::rest

