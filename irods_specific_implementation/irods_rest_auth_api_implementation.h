#ifndef IRODS_REST_CPP_AUTH_API_IMPLEMENTATION_H
#define IRODS_REST_CPP_AUTH_API_IMPLEMENTATION_H

#include "irods_rest_api_base.h"

#include <pistache/router.h>

namespace irods::rest
{
    // this is contractually tied directly to the api implementation
    const std::string service_name{"irods_rest_cpp_auth_server"};

    class auth : public api_base
    {
    public:
        auth() : api_base{service_name}
        {
            info("Endpoint initialized.");
        }

        std::tuple<Pistache::Http::Code, std::string>
        operator()(const Pistache::Rest::Request& _request,
                   Pistache::Http::ResponseWriter& _response)
        {
            try {
                auto [user_name, password, auth_type] = decode(_request.headers().getRaw("authorization").value());

                authenticate(user_name, password, auth_type);

                // use the zone key as our secret
                const auto zone_key = irods::get_server_property<std::string>(irods::KW_CFG_ZONE_KEY);
                debug("zone_key = [{}]", zone_key);

                trace("Generating JWT for user [{}] ...", user_name);
                auto token = jwt::create()
                    .set_type("JWS")
                    .set_issuer(keyword::issue_claim)
                    .set_subject(keyword::subject_claim)
                    .set_audience(keyword::audience_claim)
                    .set_not_before(std::chrono::system_clock::now())
                    .set_issued_at(std::chrono::system_clock::now())
                    // TODO: consider how to handle token revocation, token refresh
                    //.set_expires_at(std::chrono::system_clock::now() - std::chrono::seconds{30})
                    .set_payload_claim(keyword::user_name, jwt::claim(user_name))
                    .sign(jwt::algorithm::hs256{zone_key});

                return std::make_tuple(Pistache::Http::Code::Ok, token);
            }
            catch (const irods::exception& e) {
                error("Caught exception - [error_code={}] {}", e.code(), e.client_display_what());
                return make_error_response(e.code(), e.what());
            }
            catch (const std::exception& e) {
                error("Caught exception - {}", e.what());
                return make_error_response(SYS_INVALID_INPUT_PARAM, e.what());
            }
        } // operator()

    private:
        void throw_for_invalid_header(const std::string _h)
        {
            THROW(SYS_INVALID_INPUT_PARAM, fmt::format("invalid Authorization Header {}", _h));
        }

        auto base64_decode(const std::string& _in)
        {
            static constexpr unsigned char table[] = {
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
                52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
                64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
                15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
                64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
                41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
            };

            auto in_len = _in.size();

            if (in_len % 4 != 0) {
                THROW(SYS_INVALID_INPUT_PARAM, "input is not a multiple of 4");
            }

            auto out_len = in_len / 4 * 3;

            if (_in[in_len - 1] == '=') { --out_len; }
            if (_in[in_len - 2] == '=') { --out_len; }

            std::string out;
            out.resize(out_len);

            for (size_t i = 0, j = 0; i < in_len;) {
                auto a = (_in[i] == '=') ? uint32_t{0} & i++ : table[static_cast<uint32_t>(_in[i++])];
                auto b = (_in[i] == '=') ? uint32_t{0} & i++ : table[static_cast<uint32_t>(_in[i++])];
                auto c = (_in[i] == '=') ? uint32_t{0} & i++ : table[static_cast<uint32_t>(_in[i++])];
                auto d = (_in[i] == '=') ? uint32_t{0} & i++ : table[static_cast<uint32_t>(_in[i++])];
                auto t = (a << 3 * 6) + (b << 2 * 6) + (c << 1 * 6) + (d << 0 * 6);

                if (j < out_len) { out[j++] = (t >> 2 * 8) & 0xFF; }
                if (j < out_len) { out[j++] = (t >> 1 * 8) & 0xFF; }
                if (j < out_len) { out[j++] = (t >> 0 * 8) & 0xFF; }
            } // for i

            return out;
        } // base64_decode

        auto parse_header(const std::string _header) -> std::tuple<std::string, std::string>
        {
            trace("Parsing header ...");

            // TODO Don't log the base64 encoded header. It contains the client's username and password.
            // We need to determine whether logging this information is valuable.
            //debug("_header = [{}]", _header);

            const auto p0 = _header.find_first_of(" ");
            if (std::string::npos == p0) {
                throw_for_invalid_header(_header);
            }

            auto type = _header.substr(0, p0);
            auto token = _header.substr(p0 + 1);

            return std::make_tuple(type, token);
        } // parse_header

        auto decode(const std::string _header) -> std::tuple<std::string, std::string, std::string>
        {
            trace("Decoding header for auth-type, token, username, and password ...");

            // TODO Don't log the base64 encoded header. It contains the client's username and password.
            // We need to determine whether logging this information is valuable.
            //debug("_header = [{}]", _header);

            const auto [auth_type, token] = parse_header(_header);

            auto creds = base64_decode(token);
            auto user_name = creds.substr(0, creds.find_first_of(":"));
            auto password = creds.substr(creds.find_first_of(":") + 1);

            debug("user_name = [{}]", user_name);

            return std::make_tuple(user_name, password, auth_type);
        } // decode
    }; // class auth
} // namespace irods::rest

#endif // IRODS_REST_CPP_AUTH_API_IMPLEMENTATION_H

