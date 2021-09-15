#include "irods_rest_api_base.h"

#include "filesystem.hpp"
#include "rodsClient.h"
#include "irods_random.hpp"
#include "rodsErrorTable.h"

#include "pistache/http_headers.h"
#include "pistache/optional.h"

#include "fmt/format.h"

#include <chrono>
#include <string_view>

// this is contractually tied directly to the swagger api definition, and the below implementation
#define MACRO_IRODS_ACCESS_API_IMPLEMENTATION \
    Pistache::Http::Code code; \
    std::string message; \
    std::tie(code, message) = irods_access_(headers, path.get(), base, use_count, seconds_until_expiration); \
    response.send(code, message);

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
            trace("Endpoint initialized.");
        }

        std::tuple<Pistache::Http::Code, std::string>
        operator()(const Pistache::Http::Header::Collection& _headers,
                   const std::string& _logical_path,
                   const std::string& _base_url,
                   const Pistache::Optional<std::string>& _use_count,
                   const Pistache::Optional<std::string>& _seconds_until_expiration)
        {
            trace("Handling request ...");

            info("Input arguments - path=[{}], use_count=[{}], seconds_until_expiration=[{}]",
                 _logical_path, _use_count.getOrElse(""), _seconds_until_expiration.getOrElse(""));

            namespace fs = irods::experimental::filesystem;

            auto conn = get_connection(_headers.getRaw("authorization").value());

            try {
                // TODO: can we ensure this is a canonical path?
                const std::string logical_path{decode_url(_logical_path)};
                debug("Logical path = [{}]", logical_path);

                const fs::path fs_path = logical_path;
                const auto size = fs::client::data_object_size(*conn(), fs_path);
                const auto ticket_id = make_ticket_id();

                // TODO These calls could benefit from an atomic_apply_ticket_operations API plugin.
                // Each one of these calls results in at least one network call.
                create_ticket(conn, ticket_id, logical_path);
                set_ticket_use_count(conn, ticket_id, _use_count);
                set_ticket_expiration_timestamp(conn, ticket_id, _seconds_until_expiration);

                using json = nlohmann::json;

                const auto results = json::object({
                    {"headers", json::object({
                        {"irods-ticket", json::array({ticket_id})}
                    })},
                    {"url", fmt::format("{}/stream?path={}&offset=0&count={}", _base_url, logical_path, size)}
                });

                debug("Result = [{}]", results.dump());

                return std::make_tuple(Pistache::Http::Code::Ok, results.dump());
            }
            catch (const fs::filesystem_error& e) {
                error("Caught exception - [error_code={}] {}", e.code().value(), e.what());
                return make_error_response(e.code().value(), e.what());
            }
            catch (const irods::exception& e) {
                error("Caught exception - [error_code={}] {}", e.code(), e.what());
                return make_error_response(e.code(), e.what());
            }
            catch (const std::exception& e) {
                error("Caught exception - {}", e.what());
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

        void create_ticket(connection_proxy& _conn,
                           const std::string_view _ticket_id,
                           const std::string_view _logical_path) const
        {
            trace("Creating ticket [{}] ...", _ticket_id);
            rx_ticket(*_conn(), "create", _ticket_id, "read", _logical_path);
        } // create_ticket

        void set_ticket_use_count(connection_proxy& _conn,
                                  const std::string_view& _ticket_id,
                                  const Pistache::Optional<std::string>& _use_count) const
        {
            trace("Setting use-count on ticket [{}] ...", _ticket_id);

            const auto use_count = _use_count.getOrElse("0");
            debug("use_count = [{}]", use_count);
            rx_ticket(*_conn(), "mod", _ticket_id, "uses", use_count);
        } // set_ticket_use_count

        void set_ticket_expiration_timestamp(connection_proxy& _conn,
                                             const std::string_view& _ticket_id,
                                             const Pistache::Optional<std::string>& _seconds_until_expiration) const
        {
            trace("Setting expiration on ticket [{}] ...", _ticket_id);

            using std::chrono::seconds;
            using std::chrono::duration_cast;

            const auto secs_until_expiration = seconds(std::stoi(_seconds_until_expiration.getOrElse("30")));
            const auto tp = std::chrono::system_clock::now() + secs_until_expiration;
            debug("seconds_until_expiration = [{}]", secs_until_expiration.count());

            const auto end_time = fmt::format("{}", duration_cast<seconds>(tp.time_since_epoch()).count());
            rx_ticket(*_conn(), "mod", _ticket_id, "expire", end_time);
        } // set_ticket_expiration_timestamp

        void rx_ticket(RcComm& _conn,
                       std::string_view _operation,
                       std::string_view _ticket_id,
                       std::string_view _type,
                       std::string_view _value) const
        {
            trace("Invoking rcTicketAdmin() for ticket [{}] ...", _ticket_id);

            ticketAdminInp_t ticket_inp{};
            ticket_inp.arg1 = const_cast<char*>(_operation.data());
            ticket_inp.arg2 = const_cast<char*>(_ticket_id.data());
            ticket_inp.arg3 = const_cast<char*>(_type.data());
            ticket_inp.arg4 = const_cast<char*>(_value.data());

            if (const auto ec = rcTicketAdmin(&_conn, &ticket_inp); ec < 0) {
                THROW(ec, fmt::format("Received error from rcTicketAdmin for ticket [{}]", _ticket_id));
            }
        } // rx_ticket
    }; // class access
} // namespace irods::rest

