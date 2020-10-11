
#include "irods_rest_api_base.h"
#include "filesystem.hpp"
#include "rodsClient.h"
#include "irods_random.hpp"

#include <chrono>
#include <ctime>

// this is contractually tied directly to the swagger api definition, and the below implementation
#define MACRO_IRODS_ACCESS_API_IMPLEMENTATION \
    Pistache::Http::Code code; \
    std::string message; \
    std::tie(code, message) = irods_access_(headers.getRaw("Authorization").value(), path.get(), base); \
    response.send(code, message);

namespace irods::rest {
    namespace sc = std::chrono;
    using sclk = sc::system_clock;

    namespace fs   = irods::experimental::filesystem;
    namespace fcli = irods::experimental::filesystem::client;
    using     fsp  = fs::path;

    // this is contractually tied directly to the api implementation
    const std::string service_name{"irods_rest_cpp_access_server"};

    class access : public api_base {
        public:

            access() : api_base{service_name}
            {
                // ctor
            }

            std::tuple<Pistache::Http::Code &&, std::string> operator()(
                const std::string& _auth_header,
                const std::string& _logical_path,
                const std::string& _base_url) {

                auto conn = get_connection(_auth_header);
                try {
                    // TODO: can we ensure this is a canonical path?
                    std::string logical_path{decode_url(_logical_path)};

                    fsp fs_path{logical_path};
                    auto size = fcli::data_object_size(*conn(), fs_path);

                    // generate a ticket and create the JSON to return to the client
                    std::string ticket{make_ticket()};
                    nlohmann::json headers = nlohmann::json::array();
                    headers += std::string{"X-API-KEY: "}+ticket;

                    nlohmann::json results = nlohmann::json::object();
                    results["url"] = boost::str(
                                     boost::format(
                                     "%s/stream?path=%s&offset=0&limit=%d")
                                     % _base_url
                                     % logical_path
                                     % size);
                    results["headers"] = headers;

                    // create the ticket in the catalog
                    std::string create{"create"}, read{"read"};
                    rx_ticket(*conn(), create, ticket, read, logical_path);

                    // modify it to a single use
                    std::string mod{"mod"}, uses{"uses"}, one{"1"};
                    rx_ticket(*conn(), mod, ticket, uses, one);

                    // set the time limit on usage to 30 seconds
                    auto end_time_p{sclk::now()+sc::seconds(30)};
                    auto end_time_t{sclk::to_time_t(end_time_p)};
                    std::stringstream end_ss{}; end_ss << end_time_t;

                    std::string expire{"expire"}, end_time{end_ss.str()};
                    rx_ticket(*conn(), mod, ticket, expire, end_time);

                    return std::forward_as_tuple(
                            Pistache::Http::Code::Ok,
                            results.dump());
                }
                catch(const irods::exception& _e) {
                    return std::forward_as_tuple(
                            Pistache::Http::Code::Bad_Request,
                            _e.what());
                }
                catch(const std::exception& _e) {
                    return std::forward_as_tuple(
                            Pistache::Http::Code::Bad_Request,
                            _e.what());
                }
            } // operator()

        private:

            std::string make_ticket() {
                const int ticket_len = 15;
                // random_bytes must be (unsigned char[]) to guarantee that following
                // modulo result is positive (i.e. in [0, 61])
                unsigned char random_bytes[ticket_len];
                irods::getRandomBytes( random_bytes, ticket_len );

                const char character_set[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G',
                'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
                'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
                'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
                'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6',
                '7', '8', '9'};

                std::string new_ticket{};
                for ( int i = 0; i < ticket_len; ++i ) {
                    const int ix = random_bytes[i] % sizeof(character_set);
                    new_ticket += character_set[ix];
                }
                return new_ticket;
            } // make_ticket

            // cannot use const due to lack of const on ticket args
            void rx_ticket(
                rcComm_t&     _conn,
                std::string&  _operation,
                std::string&  _ticket,
                std::string&  _type,
                std::string&  _value) {

                ticketAdminInp_t ticket_inp{};
                ticket_inp.arg1 = &_operation[0];
                ticket_inp.arg2 = &_ticket[0];
                ticket_inp.arg3 = &_type[0];
                ticket_inp.arg4 = &_value[0];
                ticket_inp.arg5 = nullptr;
                ticket_inp.arg6 = nullptr;

                auto err = rcTicketAdmin(&_conn, &ticket_inp);
                if(err < 0) {
                    THROW( err,
                           std::string{"Failed to call ticket admin for ticket "}
                            + _ticket);
                }

            } // rx_ticket

    }; // class access

}; // namespace irods::rest
