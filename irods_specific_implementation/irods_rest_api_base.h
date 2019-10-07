#include "connection_pool.hpp"
#include "boost/format.hpp"
#include "json.hpp"
#include "pistache/http_defs.h"

#include "irods_kvp_string_parser.hpp"
#include "irods_auth_constants.hpp"
#include "irods_server_properties.hpp"

#include "jwt.h"

#include <tuple>
#include <iostream>
#include <algorithm>
#include <iostream>

namespace irods {
namespace rest {
class connection_handle {
    public:
        connection_handle(rcComm_t* _c) : conn_{_c} {}
        virtual ~connection_handle() { rcDisconnect(conn_); }
        rcComm_t* operator()() {
            return conn_;
        }

    private:
        rcComm_t* conn_;
}; // class connection_handle

class api_base {
    public:
    static void no_log_in(rcComm_t&) {};
    public:
    api_base() {
        rodsEnv env{};
        _getRodsEnv(env);

        connection_pool_ = std::make_shared<irods::connection_pool>(
                               3,
                               env.rodsHost,
                               env.rodsPort,
                               env.rodsUserName,
                               env.rodsZone,
                               env.irodsConnectionPoolRefreshTime,
                               no_log_in);
    } // ctor

    virtual ~api_base() {
    }

    protected:
        connection_handle get_connection() {
            auto conn = connection_pool_->get_connection();
            return connection_handle(conn.release());
        } // get_connection

        void authenticate(
            rcComm_t*         _comm,
            const std::string _header) {
            // use the zone key as our secret
            std::string zone_key{irods::get_server_property<const std::string>(irods::CFG_ZONE_KEY_KW)};

            // remove Authorization: from the string
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
            auto payload = decoded.get_payload_claims();

            const std::string& user_name = payload["user_name"].as_string();
            const std::string& password  = payload["password"].as_string();
            const std::string& auth_type = payload["auth_type"].as_string();

            // set password in context string for auth
            kvp_map_t kvp;
            kvp[irods::AUTH_PASSWORD_KEY] = password;
            kvp[irods::AUTH_USER_NAME_KEY] = user_name;
            std::string context = irods::escaped_kvp_string(kvp);

            int err = clientLogin(
                          _comm,
                          context.c_str(),
                          auth_type.c_str());
            if(err < 0) {
                THROW(err,
                    boost::format("[%s] failed to login with type [%s]")
                    % user_name
                    % auth_type);
            }
        } // authenticate

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
    std::shared_ptr<irods::connection_pool> connection_pool_;

}; // class api_base
}; // namespace rest
}; // namespace irods
