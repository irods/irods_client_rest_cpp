#include "irods_rest_api_base.h"

#include "constants.hpp"
#include <irods/filesystem.hpp>
#include <irods/rodsClient.h>
#include <irods/irods_random.hpp>
#include <irods/rodsErrorTable.h>
#include <irods/irods_at_scope_exit.hpp>
#include <irods/irods_query.hpp>

#include <pistache/http_headers.h>
#include <pistache/optional.h>
#include <pistache/router.h>

#include <boost/algorithm/string.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <string_view>
#include <iomanip>
#include <sstream>

using json = nlohmann::json;

namespace irods::rest
{
    // this is contractually tied directly to the api implementation
    const std::string service_name{"irods_rest_cpp_access_server"};

    class access : public api_base
    {
    public:
        access()
            : api_base{service_name}
        {
            info("Endpoint initialized.");
        }

        std::tuple<Pistache::Http::Code, std::string>
        operator()(const Pistache::Rest::Request& _request,
                   Pistache::Http::ResponseWriter& _response)
        {
            namespace fs = irods::experimental::filesystem;

            std::string ticket_id;

            try {
                auto _logical_path = _request.query().get("logical-path").get();
                auto _type = _request.query().get("type");
                auto _use_count = _request.query().get("use-count");
                auto _write_file_count = _request.query().get("write-file-count");
                auto _write_byte_count = _request.query().get("write-byte-count");
                auto _seconds_until_expiration = _request.query().get("seconds-until-expiration");
                auto _users = _request.query().get("users");
                auto _groups = _request.query().get("groups");
                auto _hosts = _request.query().get("hosts");

                auto conn = get_connection(_request.headers().getRaw("authorization").value());

                const fs::path logical_path = decode_url(_logical_path);
                debug("Logical path = [{}]", logical_path.c_str());

                const auto size = fs::client::data_object_size(*conn(), logical_path);
                ticket_id = make_ticket_id();

                // TODO These calls could benefit from an atomic_apply_ticket_operations API plugin.
                // Each one of these calls results in at least one network call.
                create_ticket(conn, ticket_id, _type, logical_path.c_str());
                set_ticket_use_count(conn, ticket_id, _use_count);
                set_ticket_write_file_count(conn, ticket_id, _write_file_count);
                set_ticket_write_byte_count(conn, ticket_id, _write_byte_count);
                set_ticket_expiration_timestamp(conn, ticket_id, _seconds_until_expiration);
                set_ticket_allowed_users(conn, ticket_id, _users);
                set_ticket_allowed_groups(conn, ticket_id, _groups);
                set_ticket_allowed_hosts(conn, ticket_id, _hosts);

                using json = nlohmann::json;

                const auto results = json::object({
                    {"headers", json::object({
                        {"irods-ticket", json::array({ticket_id})}
                    })},
                    {"url", fmt::format("{}/stream?logical-path={}&offset=0&count={}", base_url, logical_path.c_str(), size)}
                });

                return std::make_tuple(Pistache::Http::Code::Ok, results.dump());
            }
            catch (const fs::filesystem_error& e) {
                error("Caught exception - [error_code={}] {}", e.code().value(), e.what());
                delete_ticket(_request, ticket_id);
                return make_error_response(e.code().value(), e.what());
            }
            catch (const irods::exception& e) {
                error("Caught exception - [error_code={}] {}", e.code(), e.what());
                delete_ticket(_request, ticket_id);
                return make_error_response(e.code(), e.what());
            }
            catch (const std::exception& e) {
                error("Caught exception - {}", e.what());
                delete_ticket(_request, ticket_id);
                return make_error_response(SYS_INTERNAL_ERR, e.what());
            }
        } // operator()

    private:
        std::string make_ticket_id() const
        {
            trace("Generating ticket identifier ...");

            constexpr int ticket_len = 15;

            // random_bytes must be (unsigned char[]) to guarantee that following
            // modulo result is positive (i.e. in [0, 61])
            unsigned char random_bytes[ticket_len];
            irods::getRandomBytes(random_bytes, ticket_len);

            const char character_set[] = {
                'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
                'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
                'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7',
                '8', '9'
            };

            std::string new_ticket;
            new_ticket.reserve(ticket_len);

            for (int i = 0; i < ticket_len; ++i) {
                const int ix = random_bytes[i] % sizeof(character_set);
                new_ticket += character_set[ix];
            }

            debug("Generated ticket identifier = [{}]", new_ticket);

            return new_ticket;
        } // make_ticket_id

        void delete_ticket(const Pistache::Rest::Request& _request, const std::string_view _ticket_id) noexcept
        {
            try {
                auto conn = get_connection(_request.headers().getRaw("authorization").value());

                const auto gql = fmt::format("select TICKET_ID where ticket_string = '{}'", _ticket_id);

                if (irods::query q{conn(), gql}; q.size() == 0) {
                    return;
                }

                rx_ticket(*conn(), "create", _ticket_id);
            } catch (...) {}
        } // delete_ticket

        void create_ticket(connection_proxy& _conn,
                           const std::string_view _ticket_id,
                           const Pistache::Optional<std::string>& _ticket_type,
                           const std::string_view _logical_path) const
        {
            trace("Creating ticket [{}] ...", _ticket_id);
            rx_ticket(*_conn(), "create", _ticket_id, _ticket_type.getOrElse("read"), _logical_path);
        } // create_ticket

        void set_ticket_use_count(connection_proxy& _conn,
                                  const std::string_view& _ticket_id,
                                  const Pistache::Optional<std::string>& _use_count) const
        {
            trace("Setting use-count on ticket [{}] ...", _ticket_id);

            const auto count = _use_count.getOrElse("0");
            debug("use_count = [{}]", count);
            rx_ticket(*_conn(), "mod", _ticket_id, "uses", count);
        } // set_ticket_use_count

        void set_ticket_write_file_count(connection_proxy& _conn,
                                         const std::string_view _ticket_id,
                                         const Pistache::Optional<std::string>& _file_count)
        {
            trace("Setting write-file count on ticket [{}] ...", _ticket_id);

            const auto count = _file_count.getOrElse("0");
            debug("write_file_count = [{}]", count);
            rx_ticket(*_conn(), "mod", _ticket_id, "write-file", count);
        } // set_ticket_write_file_count

        void set_ticket_write_byte_count(connection_proxy& _conn,
                                         const std::string_view _ticket_id,
                                         const Pistache::Optional<std::string>& _byte_count)
        {
            trace("Setting write-byte count on ticket [{}] ...", _ticket_id);

            const auto count = _byte_count.getOrElse("0");
            debug("write_byte_count = [{}]", count);
            rx_ticket(*_conn(), "mod", _ticket_id, "write-bytes", count);
        } // set_ticket_write_byte_count

        void set_ticket_expiration_timestamp(connection_proxy& _conn,
                                             const std::string_view& _ticket_id,
                                             const Pistache::Optional<std::string>& _seconds_until_expiration) const
        {
            if (_seconds_until_expiration.isEmpty()) {
                return;
            }

            const auto secs_until_exp_string = _seconds_until_expiration.getOrElse("0");
            debug("seconds until expiration = [{}]", secs_until_exp_string);

            if ("0" == secs_until_exp_string) {
                return;
            }

            trace("Setting expiration on ticket [{}] ...", _ticket_id);

            const auto secs = std::stoll(secs_until_exp_string);

            if (secs < 0) {
                constexpr std::string_view msg_fmt = "seconds_until_expiration [{}] must be greater than or equal to zero.";
                THROW(SYS_INVALID_INPUT_PARAM, fmt::format(msg_fmt, secs));
            }

            using std::chrono::seconds;
            using std::chrono::system_clock;

            const auto expiration_timestamp = system_clock::now() + seconds{secs};
            const auto seconds_since_epoch = system_clock::to_time_t(expiration_timestamp);

            rx_ticket(*_conn(), "mod", _ticket_id, "expire", std::to_string(seconds_since_epoch));
        } // set_ticket_expiration_timestamp

        void set_ticket_allowed_users(connection_proxy& _conn,
                                      const std::string_view& _ticket_id,
                                      const Pistache::Optional<std::string>& _users) const
        {
            if (_users.isEmpty()) {
                return;
            }

            trace("Adding allowed users to ticket [{}] ...", _ticket_id);

            std::vector<std::string> list;
            boost::algorithm::split(list, _users.get(), boost::is_any_of(","));

            for (auto&& user : list) {
                debug("allowed user = [{}]", user);
                rx_ticket(*_conn(), "mod", _ticket_id, "add", "user", user.data());
            }
        } // set_ticket_allowed_users

        void set_ticket_allowed_groups(connection_proxy& _conn,
                                       const std::string_view& _ticket_id,
                                       const Pistache::Optional<std::string>& _groups) const
        {
            if (_groups.isEmpty()) {
                return;
            }

            trace("Adding allowed groups to ticket [{}] ...", _ticket_id);

            std::vector<std::string> list;
            boost::algorithm::split(list, _groups.get(), boost::is_any_of(","));

            for (auto&& group : list) {
                debug("allowed group = [{}]", group);
                rx_ticket(*_conn(), "mod", _ticket_id, "add", "group", group.data());
            }
        } // set_ticket_allowed_groups

        void set_ticket_allowed_hosts(connection_proxy& _conn,
                                      const std::string_view& _ticket_id,
                                      const Pistache::Optional<std::string>& _hosts) const
        {
            if (_hosts.isEmpty()) {
                return;
            }

            trace("Adding allowed hosts to ticket [{}] ...", _ticket_id);

            std::vector<std::string> list;
            boost::algorithm::split(list, _hosts.get(), boost::is_any_of(","));

            for (auto&& host : list) {
                debug("allowed host = [{}]", host);
                rx_ticket(*_conn(), "mod", _ticket_id, "add", "host", host.data());
            }
        } // set_ticket_allowed_hosts

        void rx_ticket(RcComm& _conn,
                       const std::string_view _operation,
                       const std::string_view _ticket_id,
                       const std::string_view _arg0 = "",
                       const std::string_view _arg1 = "",
                       const std::string_view _arg2 = "") const
        {
            trace("Invoking rcTicketAdmin() for ticket [{}] ...", _ticket_id);

            ticketAdminInp_t ticket_inp{};
            ticket_inp.arg1 = const_cast<char*>(_operation.data());
            ticket_inp.arg2 = const_cast<char*>(_ticket_id.data());
            ticket_inp.arg3 = const_cast<char*>(_arg0.data());
            ticket_inp.arg4 = const_cast<char*>(_arg1.data());
            ticket_inp.arg5 = const_cast<char*>(_arg2.data());

            if (const auto ec = rcTicketAdmin(&_conn, &ticket_inp); ec < 0) {
                THROW(ec, fmt::format("Received error from rcTicketAdmin for ticket [{}]", _ticket_id));
            }
        } // rx_ticket
    }; // class access
} // namespace irods::rest

