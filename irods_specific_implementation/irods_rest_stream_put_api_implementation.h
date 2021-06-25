#include "irods_rest_api_base.h"

#include "dstream.hpp"
#include "transport/default_transport.hpp"
#include "rodsErrorTable.h"
#include "irods_exception.hpp"
#include "ticketAdmin.h"
#include "indexed_connection_pool_with_expiry.hpp"

#include "pistache/http.h"
#include "pistache/optional.h"

#include <algorithm>

// this is contractually tied directly to the swagger api definition, and the below implementation
#define MACRO_IRODS_STREAM_PUT_API_IMPLEMENTATION \
    Pistache::Http::Code code; \
    std::string message; \
    std::tie(code, message) = irods_stream_put_(headers, body, path, offset, count, truncate); \
    response.send(code, message);

namespace irods::rest
{
    // this is contractually tied directly to the api implementation
    const std::string service_name{"irods_rest_cpp_stream_put_server"};

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
                   const Pistache::Optional<std::string>& _offset,
                   const Pistache::Optional<std::string>& _count,
                   const Pistache::Optional<std::string>& _truncate)
        {
            logger_->trace("Handling /stream request for write ...");

            auto conn = get_connection(_headers.getRaw("authorization").value());

            if (const auto ec = set_session_ticket_if_available(_headers, conn); ec != 0) {
                return make_error_response(ec, "Failed to initialize session with ticket");
            }

            try {
                io::client::native_transport xport(*conn());
                io::odstream ds;
                const auto decoded_path = open_replica(_path, _truncate, xport, ds);

                if (!ds.is_open()) {
                    const auto msg = fmt::format("Failed to open data object [{}]", decoded_path.c_str());
                    return make_error_response(SYS_INVALID_INPUT_PARAM, msg);
                }

                apply_offset(_offset, ds);

                if (const auto count = calculate_bytes_to_write(_body.size(), _count); count > 0) {
                    logger_->trace("Writing [{}] bytes to replica.", count);
                    ds.write(_body.c_str(), count);
                }

                return std::make_tuple(Pistache::Http::Code::Ok, SUCCESS);
            }
            catch (const irods::exception& e) {
                return make_error_response(e.code(), e.what());
            }
            catch (const std::exception& e) {
                return make_error_response(SYS_INVALID_INPUT_PARAM, e.what());
            }
        } // operator()

    private:
        std::string open_replica(const std::string& _path,
                                 const Pistache::Optional<std::string>& _truncate,
                                 io::client::native_transport& _xport,
                                 io::odstream& _stream) const
        {
            auto p = decode_url(_path);
            logger_->debug("logical_path = [{}]", p);

            logger_->debug("Opening replica for data object [{}]", p);

            // Truncates the replica unless the client instructs it not to.
            if (const auto v = _truncate.getOrElse("true"); v == "false") {
                _stream.open(_xport, p, std::ios::in | std::ios::out);
            }
            else {
                logger_->debug("Truncating replica.");
                _stream.open(_xport, p);
            }

            return p;
        } // open_replica

        void apply_offset(const Pistache::Optional<std::string>& _offset, io::odstream& _stream) const
        {
            const std::int64_t offset = std::stoll(_offset.getOrElse("0"));

            if (offset < 0) {
                THROW(SYS_INVALID_INPUT_PARAM, fmt::format("Invalid offset [{}]", offset));
            }

            if (offset > 0) {
                logger_->debug("count = [{}]", offset);
                _stream.seekp(offset);
            }
        } // apply_offset

        std::int64_t calculate_bytes_to_write(std::int64_t _buffer_size,
                                              const Pistache::Optional<std::string>& _count) const
        {
            if (_count.isEmpty()) {
                logger_->debug("Count not provided. Using size of buffer [{}]", _buffer_size);
                return _buffer_size;
            }

            const std::int64_t count = std::stoll(_count.get());

            if (count < 0) {
                THROW(SYS_INVALID_INPUT_PARAM, fmt::format("Invalid byte count [{}]", count));
            }

            return std::min(_buffer_size, count);
        } // calculate_bytes_to_write
    }; // class stream
} // namespace irods::rest

