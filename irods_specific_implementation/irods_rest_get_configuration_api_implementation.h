
#include "irods_rest_api_base.h"
#include "irods_default_paths.hpp"
#include "query_builder.hpp"

#include <boost/filesystem.hpp>

#include <fstream>

// this is contractually tied directly to the pistache implementation, and the below implementation
#define MACRO_IRODS_CONFIGURATION_GET_API_IMPLEMENTATION \
    Pistache::Http::Code code; \
    std::string message; \
    std::tie(code, message) = irods_get_configuration_(request.headers().getRaw("Authorization").value()); \
    response.send(code, message);

namespace irods::rest {
    using json = nlohmann::json;
    namespace bfs = boost::filesystem;

    // this is contractually tied directly to the api implementation
    const std::string service_name{"irods_rest_cpp_get_configuration_server"};

    class get_configuration : public api_base {
        auto throw_if_user_is_not_rodsadmin(connection_proxy& _conn) -> void
        {
            auto* user_name = _conn()->clientUser.userName;

            auto sql = fmt::format(
                           "SELECT USER_TYPE WHERE USER_NAME = '{}'",
                           user_name);

            irods::experimental::query_builder qb;

            for(const auto row : qb.build(*_conn(), sql)) {
                if("rodsadmin" != row[0]) {
                    auto msg = fmt::format("{} is not a rodsadmin user", user_name);
                    THROW(CAT_INVALID_USER, msg);
                }
            }

        } // throw_if_user_is_not_rodsadmin

        auto ends_with_dot_json(const bfs::directory_entry& _e) -> bool
        {
            const auto s = std::string{".json"};
            const auto l = _e.path().string().length();
            const auto p = _e.path().string();
            const auto x = p.substr(l-s.length());

            return  x == s;
        }

        public:

        get_configuration() : api_base{service_name}
        {
            // ctor
        }

        auto operator()(const std::string& _auth_header) -> std::tuple<Pistache::Http::Code &&, std::string>
        {

            try {
                auto conn = get_connection(_auth_header);
                throw_if_user_is_not_rodsadmin(conn);

                auto dir = get_irods_config_directory();
                auto cfg = json::array();

                for(auto&& e : bfs::recursive_directory_iterator(dir)) {

                    if(!bfs::is_regular_file(e)) {
                        continue;
                    }

                    if(!ends_with_dot_json(e)) {
                        continue;
                    }

                    try {
                        auto p = e.path().string();
                        auto f = json::parse(std::ifstream(p));
                        cfg.push_back(json{p, "parsing"});

                        auto n = e.path().filename().string();
                        cfg.push_back(json{n, f});
                    }
                    catch(const json::exception& _e) {
                        auto n = e.path().filename().string();
                        cfg.push_back(json{n, _e.what()});
                    }

                } // for e

                std::string d = cfg.dump();
                return std::forward_as_tuple(
                           Pistache::Http::Code::Ok, d);
            }
            catch(const irods::exception& _e) {
                auto error = make_error(_e.code(), _e.client_display_what());
                return std::forward_as_tuple(
                           Pistache::Http::Code::Bad_Request,
                           error);
            }

        } // operator()
    }; // class get_configuration
}; // namespace irods::rest
