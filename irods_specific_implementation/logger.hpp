#ifndef IRODS_REST_CPP_LOGGER_HPP
#define IRODS_REST_CPP_LOGGER_HPP

#include "irods_rest_api_base.h"

#include "fmt/format.h"
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

        auto primary_logger = spdlog::syslog_logger_mt(_service_name.data(), "", LOG_PID, LOG_LOCAL0, enable_formatting);
        auto json_logger = spdlog::syslog_logger_mt("json", "", LOG_PID, LOG_LOCAL0, enable_formatting);

        try {
            primary_logger->set_pattern(
                fmt::format(R"_({{"timestamp": "%Y-%m-%dT%T.%e%z", "service": "{}", )_"
                            R"_("pid": %P, "thread": %t, "severity": "%l", "message": "%v"}})_", _service_name));

            json_logger->set_pattern(
                fmt::format(R"_({{"timestamp": "%Y-%m-%dT%T.%e%z", "service": "{}", )_"
                            R"_("pid": %P, "thread": %t, "severity": "%l", "message": %v}})_", _service_name));

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
                // clang-format on

                primary_logger->set_level(lvl_enum);
                json_logger->set_level(lvl_enum);
            }
            else {
                primary_logger->set_level(spdlog::level::info);
                json_logger->set_level(spdlog::level::info);
            }
        }
        catch (...) {
            primary_logger->set_level(spdlog::level::info);
            json_logger->set_level(spdlog::level::info);
        }

        // This allows us to use the global logger without needing to look it up.
        // This is required so that the API interfaces have a logger.
        spdlog::set_default_logger(primary_logger);

        return primary_logger;
    } // init_logger
} // namespace irods::rest

#endif // IRODS_REST_CPP_LOGGER_HPP

