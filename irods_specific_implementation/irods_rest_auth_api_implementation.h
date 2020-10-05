
#include "irods_rest_api_base.h"

// this is contractually tied directly to the swagger api definition, and the below implementation
#define MACRO_IRODS_AUTH_API_IMPLEMENTATION \
    Pistache::Http::Code code; \
    std::string message; \
    irods_auth_.add_headers(response); \
    std::tie(code, message) = irods_auth_(user_name.get(), password.get(), auth_type.get()); \
    response.send(code, message);

namespace irods {
namespace rest {
class auth : public api_base {

    using json = nlohmann::json;

    public:
    std::tuple<Pistache::Http::Code &&, std::string> operator()(
        const std::string& _user_name,
        const std::string& _password,
        const std::string& _auth_type ) {

        try {

            auto conn = get_connection(_user_name, _password, _auth_type);

            // use the zone key as our secret
            std::string zone_key{irods::get_server_property<const std::string>(irods::CFG_ZONE_KEY_KW)};

            auto token = jwt::create()
                             .set_type("JWS")
                             .set_issuer(keyword::issue_claim)
                             .set_subject(keyword::subject_claim)
                             .set_audience(keyword::audience_claim)
                             .set_not_before(std::chrono::system_clock::now())
                             .set_issued_at(std::chrono::system_clock::now())
                             // TODO: consider how to handle token revocation, token refresh
                             //.set_expires_at(std::chrono::system_clock::now() - std::chrono::seconds{30})
                             .set_payload_claim(keyword::user_name, jwt::claim(std::string{_user_name}))
                             .sign(jwt::algorithm::hs256{zone_key});

            return std::forward_as_tuple(
                       Pistache::Http::Code::Ok,
                       token);
        }
        catch(const irods::exception& _e) {
            json msg{};
            msg["error_code"]    = rodsErrorName(_e.code(), nullptr);
            msg["error_message"] = boost::str(boost::format("[%s] failed to authenticate with type [%s]")
                                   % _user_name % _auth_type);

            return std::forward_as_tuple(
                       Pistache::Http::Code::Bad_Request, msg.dump());
        }

    } // operator()
}; // class auth
}; // namespace rest
}; // namespace irods
