#ifndef IRODS_REST_CPP_LOGGER_HPP
#define IRODS_REST_CPP_LOGGER_HPP

#include "irods_rest_api_base.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/syslog_sink.h"

#include <memory>
#include <string_view>

namespace irods::rest
{
    auto init_logger(const std::string_view _service_name, irods::rest::configuration& _config)
        -> std::shared_ptr<spdlog::logger>
    {
        constexpr auto enable_formatting = true;

        // Register a new syslog logger.
        auto logger = spdlog::syslog_logger_mt(_service_name.data(), "", LOG_PID, LOG_LOCAL0, enable_formatting);

        try {
            logger->set_pattern(R"_({"timestamp": "%Y-%m-%dT%T.%e%z", "service": "%n", )_"
                                R"_("pid": %P, "thread": %t, "severity": "%l", "message": "%v"})_");

            if (_config.contains(configuration_keywords::log_level)) {
                namespace keywords = configuration_keywords;

                const auto& lvl = _config.at(keywords::log_level).get_ref<const std::string&>();
                auto lvl_enum = spdlog::level::info;
                
                // clang-format off
                if      (lvl == "critical") { lvl_enum = spdlog::level::critical; }
                else if (lvl == "error")    { lvl_enum = spdlog::level::err; }
                else if (lvl == "warn")     { lvl_enum = spdlog::level::warn; }
                else if (lvl == "info")     { lvl_enum = spdlog::level::info; }
                else if (lvl == "debug")    { lvl_enum = spdlog::level::debug; }
                else if (lvl == "trace")    { lvl_enum = spdlog::level::trace; }
                else                        { lvl_enum = spdlog::level::info; }
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

    class logger
    {
    protected:
        explicit logger(const std::string_view _service_name, const configuration& _config)
            : logger_{spdlog::syslog_logger_mt(_service_name.data(), "", LOG_PID, LOG_LOCAL0, true /* enable formatting */)}
        {
            configure(_config);
        }

        logger(const logger&) = delete;
        auto operator=(const logger&) -> logger& = delete;

        virtual ~logger() = default;

        std::shared_ptr<spdlog::logger> logger_;

    private:
        void configure(const configuration& _config) noexcept
        {
            try {
                logger_->set_pattern(R"_({"timestamp": "%Y-%m-%dT%T.%e%z", "service": "%n", )_"
                                     R"_("pid": %P, "thread": %t, "severity": "%l", "message": "%v"})_");

                if (_config.contains(configuration_keywords::log_level)) {
                    namespace keywords = configuration_keywords;

                    const auto& lvl = _config.at(keywords::log_level).get_ref<const std::string&>();
                    auto lvl_enum = spdlog::level::info;
                    
                    // clang-format off
                    if      (lvl == "critical") { lvl_enum = spdlog::level::critical; }
                    else if (lvl == "error")    { lvl_enum = spdlog::level::err; }
                    else if (lvl == "warn")     { lvl_enum = spdlog::level::warn; }
                    else if (lvl == "info")     { lvl_enum = spdlog::level::info; }
                    else if (lvl == "debug")    { lvl_enum = spdlog::level::debug; }
                    else if (lvl == "trace")    { lvl_enum = spdlog::level::trace; }
                    else                        { lvl_enum = spdlog::level::info; }
                    // clang-format on

                    logger_->set_level(lvl_enum);
                }
                else {
                    logger_->set_level(spdlog::level::info);
                }
            }
            catch (...) {
                logger_->set_level(spdlog::level::info);
            }
        } // configure
    }; // class logger
} // namespace irods::rest

#endif // IRODS_REST_CPP_LOGGER_HPP

