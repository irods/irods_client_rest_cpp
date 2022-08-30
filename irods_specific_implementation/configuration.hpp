#ifndef IRODS_REST_CPP_CONFIGURATION_HPP
#define IRODS_REST_CPP_CONFIGURATION_HPP

#include <nlohmann/json.hpp>

#include <string>
#include <string_view>

namespace irods::rest::configuration
{
    /// \brief Initializes the global JSON configuration object.
    ///
    /// \throws std::runtime_exception If opening/reading the JSON configuration fails.
    /// \throws nlohmann::json::exception If parsing the JSON configuration fails.
    auto init() -> void;

    /// \brief Returns the JSON object attached to the "rest_api" key in the JSON configuration.
    ///
    /// \throws json::exception If the "rest_api" key does not exist at the top of the JSON object.
    auto rest_api() -> const nlohmann::json&;

    /// \brief Returns the JSON object attached to the \p _service_name in "rest_api" the JSON configuration.
    //
    /// \param[in] _service_name Name of the target REST service.
    ///
    /// \throws json::exception If the "rest_api" key does not exist or \p _service_name does not exist under
    /// "rest_api".
    auto rest_service(std::string_view _service_name) -> const nlohmann::json&;

    /// \brief Returns the JSON object attached to the "irods_client_environment" key in the JSON configuration.
    ///
    /// \throws json::exception If the "irods_client_environment" key does not exist at the top of the JSON object.
    auto irods_client_environment() -> const nlohmann::json&;

    /// \brief Returns the JWT signing key for communications with the REST services.
    ///
    /// \throws json::exception If "rest_api" key does not exist or "jwt_signing_key" does not exist under "rest_api".
    auto get_jwt_signing_key() -> const std::string&;
} // namespace irods::rest::configuration

#endif // IRODS_REST_CPP_CONFIGURATION_HPP
