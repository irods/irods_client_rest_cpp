#ifndef IRODS_REST_CPP_LOGGER_HPP
#define IRODS_REST_CPP_LOGGER_HPP

#include "irods_rest_api_base.h"

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/syslog_sink.h>

#include <memory>
#include <string_view>

namespace irods::rest
{
    /// \brief Initialize logging for the REST service specified by \p _service_name.
    ///
    /// The logging module is initialized based on the level set in the JSON configuration
    /// for the specified \p _service_name. If none is specified, the log level is set to "info".
    ///
    /// \param[in] _service_name Key for JSON configuration for the target REST service
    auto init_logger(const std::string_view _service_name) -> std::shared_ptr<spdlog::logger>
    {
        constexpr auto enable_formatting = true;

        auto logger = spdlog::syslog_logger_mt(_service_name.data(), "", LOG_PID, LOG_LOCAL0, enable_formatting);

        try {
            // The log message format does not include whitespace because that is how
            // the nlohmann JSON library outputs JSON without indentation. This makes
            // the output appear as if it was generated in realtime rather than being
            // formatted to appear as JSON.
            logger->set_pattern(R"_({"timestamp":"%Y-%m-%dT%T.%e%z","service":"%n",)_"
                                R"_("pid":%P,"thread":%t,"severity":"%l","payload":%v})_");

            const auto& cfg = configuration::rest_service(_service_name);

            if (const auto lvl_itr = cfg.find(configuration_keywords::log_level); lvl_itr != cfg.end()) {
                namespace keywords = configuration_keywords;

                const auto& lvl = lvl_itr->get_ref<const std::string&>();
                auto lvl_enum = spdlog::level::info;

                // clang-format off
                if      (lvl == "critical") { lvl_enum = spdlog::level::critical; }
                else if (lvl == "error")    { lvl_enum = spdlog::level::err; }
                else if (lvl == "warn")     { lvl_enum = spdlog::level::warn; }
                else if (lvl == "info")     { lvl_enum = spdlog::level::info; }
                else if (lvl == "debug")    { lvl_enum = spdlog::level::debug; }
                else if (lvl == "trace")    { lvl_enum = spdlog::level::trace; }
                // clang-format on

                logger->set_level(lvl_enum);
            }
            else {
                logger->set_level(spdlog::level::info);
            }
        }
        catch (...) {
            logger->set_level(spdlog::level::info);
        }

        // This allows us to use the global logger without needing to look it up.
        // This is required so that the API interfaces have a logger.
        spdlog::set_default_logger(logger);

        return logger;
    } // init_logger
} // namespace irods::rest

#endif // IRODS_REST_CPP_LOGGER_HPP

