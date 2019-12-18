#include "connection_pool.hpp"
#include "boost/format.hpp"
#include "json.hpp"
#include "pistache/http_defs.h"

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

namespace irods {
namespace rest {
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

    auto operator[](const std::string& key) {
        return configuration_[instance_name_][key];
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

class connection_handle {
    public:
        connection_handle() {
            rodsEnv env;
            _getRodsEnv(env);
            conn_ = _rcConnect(
                        env.rodsHost,
                        env.rodsPort,
                        env.rodsUserName,
                        env.rodsZone,
                        env.rodsUserName,
                        env.rodsZone,
                        &err_,
                        0,
                        NO_RECONN);
            std::cout << "Connection Handle:" << conn_ << "\n";
        } // ctor

        connection_handle(
            const std::string& _user_name) {
            rodsEnv env;
            _getRodsEnv(env);
            conn_ = _rcConnect(
                        env.rodsHost,
                        env.rodsPort,
                        env.rodsUserName,
                        env.rodsZone,
                        _user_name.c_str(),
                        env.rodsZone,
                        &err_,
                        0,
                        NO_RECONN);
        } // ctor

        connection_handle(rcComm_t* _c) : conn_{_c} {}

        virtual ~connection_handle() { rcDisconnect(conn_); }
        rcComm_t* operator()() {
            std::cout << "Connection Handle:" << conn_ << "\n";
            return conn_;
        }

    private:
        rErrMsg_t  err_;
        rcComm_t*  conn_;
}; // class connection_handle

class api_base {
    public:
    api_base() {   } // ctor
    virtual ~api_base() {   } // dtor

    protected:
        const std::string USER_NAME_KW{"user_name"};
        const std::string ISSUE_CLAIM{"irods-rest"};
        const std::string SUBJECT_CLAIM{"irods-rest"};
        const std::string AUDIENCE_CLAIM{"irods-rest"};

        connection_handle get_connection() {
            auto conn_hdl = connection_handle();
            int err = clientLogin(conn_hdl());
            if(err < 0) {
                THROW(err,
                    boost::format("[%s] failed to login")
                    % conn_hdl()->clientUser.userName);
            }
            return conn_hdl;
        } // get_connection

        connection_handle get_connection(
            const std::string& _header) {
            auto conn_hdl = connection_handle(decode_token(_header));
            int err = clientLogin(conn_hdl());
            if(err < 0) {
                THROW(err,
                    boost::format("[%s] failed to login")
                    % conn_hdl()->clientUser.userName);
            }
            return conn_hdl;
        } // get_connection

        std::string decode_token(
            const std::string& _header) {
            // use the zone key as our secret
            std::string zone_key{irods::get_server_property<const std::string>(irods::CFG_ZONE_KEY_KW)};

            // remove Authorization: from the string
            // TODO: should we verify use of 'Bearer:' ?
            std::string token = _header.substr(_header.find(":")+1);

            // chomp the spaces
            token.erase(
                std::remove_if(
                    token.begin(),
                    token.end(),
                    [](unsigned char x){return std::isspace(x);}),
                token.end());

            // decode the token
            auto decoded = jwt::decode(token);
            auto verifier = jwt::verify()
                                .allow_algorithm(jwt::algorithm::hs256{zone_key})
                                .with_issuer(ISSUE_CLAIM);
            verifier.verify(decoded);
            auto payload = decoded.get_payload_claims();
            return payload[USER_NAME_KW].as_string();
        } // decode_token

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

}; // class api_base
}; // namespace rest
}; // namespace irods
