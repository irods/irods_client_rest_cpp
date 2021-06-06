
#include "irods_rest_api_base.h"
#include "irods_default_paths.hpp"
#include "query_builder.hpp"

#include <boost/filesystem.hpp>

#include <fstream>

// this is contractually tied directly to the pistache implementation, and the below implementation
#define MACRO_IRODS_CONFIGURATION_GET_API_IMPLEMENTATION \
    Pistache::Http::Code code; \
    std::string message; \
    std::tie(code, message) = irods_get_configuration_(request.headers().getRaw("X-API-KEY").value()); \
    response.send(code, message);

namespace irods::rest {
    using json = nlohmann::json;
    namespace bfs = boost::filesystem;

    // this is contractually tied directly to the api implementation
    const std::string service_name{"irods_rest_cpp_get_configuration_server"};

    class get_configuration : public api_base {
        auto throw_if_key_is_invalid(const std::string& _api_key) -> void
        {
            namespace ir   = irods::rest;
            namespace irck = irods::rest::configuration_keywords;

            auto cfg = ir::configuration{ir::service_name};

            if(!cfg.contains(irck::api_key)) {
                THROW(SYS_INVALID_INPUT_PARAM,
                      "GetConfiguration : API Key is not configured");
            }

            const auto key = cfg.at(irck::api_key).get<std::string>();

            if(_api_key != key) {
                THROW(SYS_INVALID_INPUT_PARAM,
                      std::string{fmt::format("GetConfiguration : Invalid API Key {} vs {}", _api_key, key)});
            }

        } // throw_if_key_is_invalid

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

        auto operator()(const std::string& _api_key) -> std::tuple<Pistache::Http::Code &&, std::string>
        {

            try {
                throw_if_key_is_invalid(_api_key);

                auto dir = get_irods_config_directory();
                auto cfg = json::object();

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
                        auto n = e.path().filename().string();
                        cfg[n] = f;
                    }
                    catch(const json::exception& _e) {
                        auto n = e.path().filename().string();
                        cfg[n] = _e.what();
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
