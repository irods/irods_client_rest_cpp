#include "irods_rest_api_base.h"

#include "dstream.hpp"
#include "transport/default_transport.hpp"
#include "rodsErrorTable.h"
#include "irods_exception.hpp"
#include "ticketAdmin.h"

#include "pistache/http.h"
#include "pistache/optional.h"

#include <vector>
#include <iterator>

// this is contractually tied directly to the swagger api definition, and the below implementation
#define MACRO_IRODS_STREAM_GET_API_IMPLEMENTATION \
    Pistache::Http::Code code; \
    std::string message; \
    std::tie(code, message) = irods_stream_get_(headers, body, path, count, offset); \
    response.send(code, message);

namespace irods::rest
{
    // this is contractually tied directly to the api implementation
    const std::string service_name{"irods_rest_cpp_stream_get_server"};

    namespace ix = irods::experimental;
    namespace fs = ix::filesystem;
    namespace io = ix::io;

    class stream : public api_base
    {
    public:
        stream()
            : api_base{service_name}
        {
            logger_->trace("Endpoint [{}] initialized.", service_name);
        }

        std::tuple<Pistache::Http::Code, std::string>
        operator()(const Pistache::Http::Header::Collection& _headers,
                   const std::string& _body,
                   const std::string& _path,
                   const std::string& _count,
                   const Pistache::Optional<std::string>& _offset)
        {
            logger_->trace("Handling /stream request for read ...");

            auto conn = get_connection(_headers.getRaw("authorization").value());

            if (const auto ec = set_session_ticket_if_available(_headers, conn); ec != 0) {
                return make_error_response(ec, "Failed to initialize session with ticket");
            }

            try {
                const fs::path path = decode_url(_path);
                logger_->debug("logical_path = [{}]", path.c_str());

                // The value returned by this function call is limited to a signed 32-bit
                // integer. This data type is used as a guard against users accidentally
                // allocating too much memory. 32 bits provides a wide range of values for
                // the buffer size.
                const auto bytes_to_read = get_number_of_bytes_to_read(_count);
                logger_->debug("count (effective) = [{}]", bytes_to_read);

                if (bytes_to_read < 0) {
                    const auto msg = fmt::format("Invalid byte count [{}]", bytes_to_read);
                    return make_error_response(SYS_INVALID_INPUT_PARAM, msg);
                }

                io::client::native_transport xport(*conn());
                io::idstream ds{xport, path};

                if (!ds.is_open()) {
                    const auto msg = fmt::format("Failed to open data object [{}]", path.c_str());
                    return make_error_response(SYS_INVALID_INPUT_PARAM, msg);
                }

                if (const std::int64_t offset = std::stoll(_offset.getOrElse("0")); offset > 0) {
                    logger_->debug("offset = [{}]", offset);
                    ds.seekg(offset);
                }
                else if (offset < 0) {
                    const auto msg = fmt::format("Invalid offset [{}]", offset);
                    return make_error_response(SYS_INVALID_INPUT_PARAM, msg);
                }

                logger_->trace("Allocating {}-byte buffer for read ...", bytes_to_read);
                std::vector<char> buffer(bytes_to_read);

                ds.read(buffer.data(), buffer.size());

                return std::make_tuple(Pistache::Http::Code::Ok, std::string(buffer.data(), ds.gcount()));
            }
            catch (const irods::exception& e) {
                return make_error_response(e.code(), e.what());
            }
            catch (const std::exception& e) {
                return make_error_response(SYS_INVALID_INPUT_PARAM, e.what());
            }
        } // operator()

    private:
        std::int32_t get_number_of_bytes_to_read(const std::string& _count) const
        {
            logger_->debug("count (requested) = [{}]", _count);

            if (_count.empty()) {
                THROW(SYS_INVALID_INPUT_PARAM, fmt::format("Invalid byte count [count parameter is required]"));
            }

            try {
                return std::stoi(_count);
            }
            catch (...) {
                THROW(SYS_INVALID_INPUT_PARAM, fmt::format("Invalid byte count [{}]", _count));
            }
        }
    }; // class stream
} // namespace irods::rest

