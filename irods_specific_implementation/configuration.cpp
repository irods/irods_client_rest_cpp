#include "configuration.hpp"

#include <fmt/format.h>

#include <fstream>

namespace
{
    using json = nlohmann::json;

    auto load() -> json
    {
        static const std::string default_configuration_file{"/etc/irods_client_rest_cpp/irods_client_rest_cpp.json"};

        std::ifstream ifs(default_configuration_file);

        if (ifs.is_open()) {
            return json::parse(ifs);
        }

        throw std::runtime_error{fmt::format("Could not load file [{}]", default_configuration_file)};
    } // load

    // Global variable for holding JSON configuration.
    json configuration;
} // anonymous namespace

namespace irods::rest::configuration
{
    auto init() -> void
    {
        ::configuration = load();
    } // init

    auto rest_api() -> const json&
    {
        return ::configuration.at("rest_api");
    } // rest_api

    auto rest_service(std::string_view _service_name) -> const json&
    {
        return rest_api().at(_service_name.data());
    } // rest_service

    auto irods_client_environment() -> const json&
    {
        return ::configuration.at("irods_client_environment");
    } // irods_client_environment

    auto get_jwt_signing_key() -> const std::string&
    {
        return rest_api().at("jwt_signing_key").get_ref<const std::string&>();
    } // get_jwt_signing_key
} // namespace irods::rest::configuration
