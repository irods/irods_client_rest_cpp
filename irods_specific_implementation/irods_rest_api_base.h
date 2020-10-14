#include "rodsClient.h"
#include "rcConnect.h"
#include "fmt/format.h"
#include "json.hpp"
#include "pistache/http_defs.h"

#include "irods_native_auth_object.hpp"
#include "irods_kvp_string_parser.hpp"
#include "irods_auth_constants.hpp"
#include "irods_server_properties.hpp"
#include "irods_exception.hpp"

#include "jwt.h"
#include "json.hpp"

#include <tuple>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <iostream>

#include "indexed_connection_pool_with_expiry.hpp"

namespace irods::rest {

    using icp = indexed_connection_pool_with_expiry;

    namespace {
        const std::string SUCCESS{"{\"code\" : 0, \"message\" : \"Success\"}"};

        auto make_error(int32_t _code, const std::string& _msg) -> std::string
        {
            return fmt::format("{{\"error_code\" : {}, \"error_message\" : \"{}\"}}", _code, _msg);

        } // make_error

        namespace configuration_keywords {
            const std::string timeout{"maximum_idle_timeout_in_seconds"};
            const std::string threads{"threads"};
            const std::string port{"port"};
        };

    } // namespace

    class configuration {
        using json = nlohmann::json;

        public:
            configuration(const std::string& instance_name) :
                instance_name_{instance_name},
                configuration_{load(DEFAULT_CONFIGURATION_FILE)}
            {
            }

            configuration(
                const std::string& instance_name,
                const std::string& file_name) :
                instance_name_{instance_name},
                configuration_{load(file_name)}
            {
            }

            auto contains(const std::string& _key) -> bool
            {
                    return configuration_.at(instance_name_).contains(_key);
            }

            auto at(const std::string& _key) -> json
            {
                try {
                    return configuration_.at(instance_name_).at(_key);
                }
                catch(...) {
                    return json{};
                }
            }

            auto operator[](const std::string& _key) -> json
            {
                try {
                    return configuration_.at(instance_name_).at(_key);
                }
                catch(...) {
                    return json{};
                }
            }

        private:
            const std::string DEFAULT_CONFIGURATION_FILE{"/etc/irods/irods_client_rest_cpp.json"};

            auto load(const std::string& file_name) -> json
            {
                std::ifstream ifs(file_name);

                if(ifs.is_open()) {
                    return json::parse(ifs);
                }

                return {};
            } // load

            const std::string instance_name_;
            const json        configuration_;

    }; // class configuration


    class api_base {

        public:
            api_base() : connection_pool_{}
            {
                load_client_api_plugins();

            } // ctor

            api_base(const std::string& _sn) : connection_pool_{}
            {
                // sets the client name for the ips command
                setenv(SP_OPTION, _sn.c_str(), 1);

                auto cfg = configuration{_sn};
                auto it  = default_idle_time_in_seconds;
                if(cfg.contains(configuration_keywords::timeout)) {
                    it = cfg.at(configuration_keywords::timeout).get<uint32_t>();
                }

                connection_pool_.set_idle_timeout(it);

                load_client_api_plugins();

            } // ctor

            virtual ~api_base()
            {
            } // dtor

        protected:

            auto  authenticate(
                  const std::string& _user_name
                , const std::string& _password
                , const std::string& _auth_type) -> void
            {
                std::string auth_type = _auth_type;
                std::transform(
                    auth_type.begin(),
                    auth_type.end(),
                    auth_type.begin(),
                    ::tolower );

                // translate standard rest auth type
                if("basic" == auth_type) {
                    auth_type = "native";
                }

                if("native" != auth_type) {
                    THROW(SYS_INVALID_INPUT_PARAM, "Only basic (irods native) authentication is supported");
                }

                auto conn = connection_handle(_user_name, _user_name);
                auto err  = clientLoginWithPassword(conn.get(), const_cast<char*>(_password.c_str()));

                if(err < 0) {
                    THROW(err,
                        fmt::format("[{}] failed to login with type [{}]"
                        , _user_name
                        , _auth_type));
                }

            } // authenticate

            auto get_connection(const std::string& _header,
                                const std::string& _hint = icp::do_not_cache_hint) -> connection_proxy
            {
                // remove Authorization: from the string, the key is the
                // Authorization header which contains a JWT
                std::string jwt = _header.substr(_header.find(":")+1);

                // chomp the spaces
                jwt.erase(
                    std::remove_if(
                        jwt.begin(),
                        jwt.end(),
                        [](unsigned char x){return std::isspace(x);}),
                    jwt.end());

                auto conn = connection_pool_.get(jwt, _hint);

                auto* ptr = conn();

                int err = clientLogin(ptr);
                if(err < 0) {
                    THROW(err,
                        fmt::format("[{}] failed to login"
                        , conn()->clientUser.userName));
                }

                return conn;

            } // get_connection

            std::string decode_url(const std::string& _in) {
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
                                THROW(
                                    SYS_INVALID_INPUT_PARAM,
                                    "Failed to decode URL");
                            }
                        }
                        else {
                            THROW(
                                SYS_INVALID_INPUT_PARAM,
                                "Failed to decode URL");
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

        private:
            icp connection_pool_;

    }; // class api_base

}; // namespace irods::rest
