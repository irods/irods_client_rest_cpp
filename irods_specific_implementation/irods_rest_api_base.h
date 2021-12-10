#ifndef IRODS_REST_CPP_API_BASE_H
#define IRODS_REST_CPP_API_BASE_H

#include "indexed_connection_pool_with_expiry.hpp"

#include "rodsClient.h"
#include "rcConnect.h"
#include "irods_native_auth_object.hpp"
#include "irods_kvp_string_parser.hpp"
#include "irods_auth_constants.hpp"
#include "irods_server_properties.hpp"
#include "user_administration.hpp"
#include "irods_exception.hpp"

#include "fmt/format.h"
#include <nlohmann/json.hpp>
#include "jwt.h"
#include "pistache/http_defs.h"
#include "pistache/http_headers.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/syslog_sink.h"

#include <tuple>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

using json = nlohmann::json;

namespace irods::rest
{
    using icp = indexed_connection_pool_with_expiry;

    namespace
    {
        const std::string SUCCESS{"{\"code\": 0, \"message\": \"Success\"}"};

        std::string make_error(std::int32_t _code, const std::string_view _msg)
        {
            return json{{"error_code", _code}, {"error_message", _msg}}.dump();
        } // make_error

        namespace configuration_keywords
        {
            const std::string timeout{"maximum_idle_timeout_in_seconds"};
            const std::string threads{"threads"};
            const std::string port{"port"};
            const std::string log_level{"log_level"};
        }
    } // namespace

    class configuration
    {
    public:
        configuration(const std::string& instance_name)
            : instance_name_{instance_name}
            , configuration_{load(DEFAULT_CONFIGURATION_FILE)}
        {
        }

        configuration(const std::string& instance_name,
                      const std::string& file_name)
            : instance_name_{instance_name}
            , configuration_{load(file_name)}
        {
        }

        auto instance_name() const noexcept -> const std::string&
        {
            return instance_name_;
        }

        auto contains(const std::string& _key) const -> bool
        {
            return configuration_.at(instance_name_).contains(_key);
        }

        auto at(const std::string& _key) const -> const json&
        {
            return configuration_.at(instance_name_).at(_key);
        }

        auto operator[](const std::string& _key) const -> const json&
        {
                return configuration_.at(instance_name_).at(_key);
        }

    private:
        const std::string DEFAULT_CONFIGURATION_FILE{"/etc/irods/irods_client_rest_cpp.json"};

        auto load(const std::string& file_name) -> json
        {
            std::ifstream ifs(file_name);

            if (ifs.is_open()) {
                return json::parse(ifs);
            }

            throw std::runtime_error{fmt::format("Could not load file [{}]", file_name)};
        } // load

        const std::string instance_name_;
        const json        configuration_;
    }; // class configuration

    class api_base
    {
    public:
        api_base(const std::string& _service_name)
            //: logger_{spdlog::syslog_logger_mt(_service_name, "", LOG_PID, LOG_LOCAL0, true /* enable formatting */)}
            : logger_{spdlog::get(_service_name)}
            , connection_pool_{}
        {
            // sets the client name for the ips command
            setenv(SP_OPTION, _service_name.c_str(), 1);

            auto cfg = configuration{_service_name};

            //configure_logger(cfg);

            auto it = default_idle_time_in_seconds;
            if (cfg.contains(configuration_keywords::timeout)) {
                it = cfg.at(configuration_keywords::timeout).get<uint32_t>();
            }

            connection_pool_.set_idle_timeout(it);

            load_client_api_plugins();
        } // ctor

        virtual ~api_base()
        {
        } // dtor

    protected:
        template <typename ...Args>
        void trace(const std::string_view _fmt, Args&&... _args) const
        {
            logger_->trace(json{{"message", fmt::format(_fmt, std::forward<Args>(_args)...)}}.dump());
        } // trace

        template <typename ...Args>
        void debug(const std::string_view _fmt, Args&&... _args) const
        {
            logger_->debug(json{{"message", fmt::format(_fmt, std::forward<Args>(_args)...)}}.dump());
        } // debug

        template <typename ...Args>
        void info(const std::string_view _fmt, Args&&... _args) const
        {
            logger_->info(json{{"message", fmt::format(_fmt, std::forward<Args>(_args)...)}}.dump());
        } // info

        template <typename ...Args>
        void warn(const std::string_view _fmt, Args&&... _args) const
        {
            logger_->warn(json{{"message", fmt::format(_fmt, std::forward<Args>(_args)...)}}.dump());
        } // warn

        template <typename ...Args>
        void error(const std::string_view _fmt, Args&&... _args) const
        {
            logger_->error(json{{"message", fmt::format(_fmt, std::forward<Args>(_args)...)}}.dump());
        } // error

        template <typename ...Args>
        void critical(const std::string_view _fmt, Args&&... _args) const
        {
            logger_->critical(json{{"message", fmt::format(_fmt, std::forward<Args>(_args)...)}}.dump());
        } // critical

        auto authenticate(const std::string& _user_name,
                          const std::string& _password,
                          const std::string& _auth_type) -> void
        {
            trace("Performing authentication for user [{}] ...", _user_name);

            std::string auth_type = _auth_type;
            std::transform(auth_type.begin(), auth_type.end(), auth_type.begin(), ::tolower);

            // translate standard rest auth type
            if ("basic" == auth_type) {
                auth_type = "native";
            }

            if ("native" != auth_type) {
                THROW(SYS_INVALID_INPUT_PARAM, "Only basic (irods native) authentication is supported");
            }

            auto conn = connection_handle(_user_name, _user_name);
            const auto ec = clientLoginWithPassword(conn.get(), const_cast<char*>(_password.c_str()));

            if (ec < 0) {
                THROW(ec, fmt::format("[{}] failed to login with type [{}]" , _user_name , _auth_type));
            }
        } // authenticate

        auto get_connection(const std::string& _header,
                            const std::string& _hint = icp::do_not_cache_hint) -> connection_proxy
        {
            trace("Getting connection to iRODS server ...");
            trace("Extracting JWT from authorization header ...");

            // remove Authorization: from the string, the key is the
            // Authorization header which contains a JWT
            std::string jwt = _header.substr(_header.find(":") + 1);

            // chomp the spaces
            jwt.erase(
                std::remove_if(
                    jwt.begin(),
                    jwt.end(),
                    [](unsigned char x) { return std::isspace(x); }),
                jwt.end());

            trace("Getting iRODS connection from pool ...");
            auto conn = connection_pool_.get(jwt, _hint);
            auto* ptr = conn();

            trace("Invoking clientLogin() ...");
            if (const int ec = clientLogin(ptr); ec < 0) {
                THROW(ec, fmt::format("[{}] failed to login" , conn()->clientUser.userName));
            }

            trace("Returning connection ...");
            return conn;
        } // get_connection

        std::string decode_url(const std::string& _in) const
        {
            trace("Decoding input [{}] ...", _in);

            std::string out;
            out.reserve(_in.size());

            for (std::size_t i = 0; i < _in.size(); ++i) {
                if (_in[i] == '%') {
                    if (i + 3 <= _in.size()) {
                        int value = 0;
                        std::istringstream is(_in.substr(i + 1, 2));
                        if (is >> std::hex >> value) {
                            out += static_cast<char>(value);
                            i += 2;
                        }
                        else {
                            THROW(SYS_INVALID_INPUT_PARAM, "Failed to decode URL");
                        }
                    }
                    else {
                        THROW(SYS_INVALID_INPUT_PARAM, "Failed to decode URL");
                    }
                }
                else if (_in[i] == '+') {
                    out += ' ';
                }
                else {
                    out += _in[i];
                }
            }

            return out;
        } // decode_url

        int set_session_ticket_if_available(const Pistache::Http::Header::Collection& _headers,
                                            connection_proxy& _conn)
        {
            trace("Setting session ticket if available ...");

            if (const auto h = _headers.tryGetRaw("irods-ticket"); !h.isEmpty()) {
                ticketAdminInp_t input{};
                input.arg1 = const_cast<char*>("session");
                input.arg2 = const_cast<char*>(h.get().value().data());
                input.arg3 = const_cast<char*>("");
                input.arg4 = const_cast<char*>("");
                input.arg5 = const_cast<char*>("");
                input.arg6 = const_cast<char*>("");

                trace("Invoking rcTicketAdmin() ...");
                return rcTicketAdmin(_conn(), &input);
            }
            else {
                trace("No session ticket information available.");
            }

            return 0;
        } // set_session_ticket_if_available

        void throw_if_user_is_not_rodsadmin(connection_proxy& _conn)
        {
            namespace adm = irods::experimental::administration;

            const auto& user = _conn()->clientUser;

            try {
                const auto type = adm::client::type(*_conn(), adm::user{user.userName, user.rodsZone});

                if (type && adm::user_type::rodsadmin != *type) {
                    if (std::strlen(user.rodsZone) > 0) {
                        const auto* msg_fmt = "{}#{} is not a rodsadmin user";
                        THROW(CAT_INVALID_USER, fmt::format(msg_fmt, user.userName, user.rodsZone));
                    }

                    THROW(CAT_INVALID_USER, fmt::format("{} is not a rodsadmin user", user.userName));
                }
            }
            catch (const adm::user_management_error&) {
                // TODO Log the error message once the class access specifier is fixed for
                // the user_management_error class.
                const auto* msg_fmt = "Encountered error while checking if user [{}] is a rodsadmin.";
                THROW(SYS_INTERNAL_ERR, fmt::format(msg_fmt, user.userName));
            }
        } // throw_if_user_is_not_rodsadmin

        auto make_error_response(int _error_code, const std::string_view _error_msg) const
        {
            const auto error = make_error(_error_code, _error_msg);
            return std::make_tuple(Pistache::Http::Code::Bad_Request, error);
        }

        std::shared_ptr<spdlog::logger> logger_;

    private:
        void configure_logger(const configuration& _cfg) noexcept
        {
            try {
                logger_->set_pattern(R"_({"timestamp": "%Y-%m-%dT%T.%e%z", "service": "%n", )_"
                                     R"_("pid": %P, "thread": %t, "severity": "%l", "message": "%v"})_");

                if (_cfg.contains(configuration_keywords::log_level)) {
                    const auto lvl = _cfg.at(configuration_keywords::log_level).get<std::string>();
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
        } // configure_logger

        icp connection_pool_;
    }; // class api_base
} // namespace irods::rest

#endif // IRODS_REST_CPP_API_BASE_H
