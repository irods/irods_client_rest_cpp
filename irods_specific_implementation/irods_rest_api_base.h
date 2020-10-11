#include "rodsClient.h"
#include "rcConnect.h"
#include "boost/format.hpp"
#include "json.hpp"
#include "pistache/http_defs.h"

#include "irods_native_auth_object.hpp"
#include "irods_kvp_string_parser.hpp"
#include "irods_auth_constants.hpp"
#include "irods_server_properties.hpp"

#include "jwt.h"
#include "json.hpp"

#include <tuple>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <iostream>

#include "indexed_connection_pool_with_expiry.hpp"

namespace irods::rest {

    namespace configuration_keywords {
        const std::string timeout{"maximum_idle_timeout_in_seconds"};
        const std::string threads{"threads"};
        const std::string port{"port"};
    };

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

            auto operator[](const std::string& key) -> json
            {
                try {
                    return configuration_[instance_name_][key];
                }
                catch(...) {
                    return json{};
                }
            }

        private:
            const std::string DEFAULT_CONFIGURATION_FILE{"/etc/irods/irods_client_rest_cpp.json"};

            json load(const std::string& file_name)
            {
                std::ifstream ifs(file_name);
                return json::parse(ifs);
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
                auto val = cfg[configuration_keywords::timeout];
                auto it  = val.empty() ? default_idle_time_in_seconds : val.get<uint32_t>();

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

                std::string password = _password;
                std::transform(
                    password.begin(),
                    password.end(),
                    password.begin(),
                    ::tolower );

                if("native" != _auth_type) {
                    THROW(SYS_INVALID_INPUT_PARAM, "Only native authentication is supported");
                }

                auto conn = connection_handle(_user_name, _user_name);

                int err = clientLoginWithPassword(conn.get(), const_cast<char*>(_password.c_str()));
                if(err < 0) {
                    THROW(err,
                        boost::format("[%s] failed to login with type [%s]")
                        % _user_name
                        % _auth_type);
                }

            } // authenticate

            auto get_connection(const std::string& _header) -> connection_proxy
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

                auto conn = connection_pool_.get(jwt);

                auto* ptr = conn();

                int err = clientLogin(ptr);
                if(err < 0) {
                    THROW(err,
                        boost::format("[%s] failed to login")
                        % conn()->clientUser.userName);
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
            indexed_connection_pool_with_expiry connection_pool_;

    }; // class api_base

}; // namespace irods::rest
