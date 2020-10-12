
#include "irods_rest_api_base.h"

#include "dstream.hpp"
#include "transport/default_transport.hpp"

// this is contractually tied directly to the swagger api definition, and the below implementation
#define MACRO_IRODS_STREAM_PUT_API_IMPLEMENTATION \
    Pistache::Http::Code code; \
    std::string message; \
    std::tie(code, message) = irods_stream_put_(headers.getRaw("Authorization").value(), body, path.get(), offset.get(), limit.get()); \
    response.send(code, message);

namespace irods::rest {

    // this is contractually tied directly to the api implementation
    const std::string service_name{"irods_rest_cpp_stream_put_server"};

    namespace ix = irods::experimental;
    namespace fs = ix::filesystem;
    namespace io = ix::io;

    class stream : public api_base {
        public:
            stream() : api_base{service_name}
            {
                // ctor
            }

            std::tuple<Pistache::Http::Code &&, std::string> operator()(
                const std::string& _auth_header,
                const std::string& _body,
                const std::string& _path,
                const std::string& _offset,
                const std::string& _limit) {

                auto conn = get_connection(_auth_header);

                try {
                    auto path   = fs::path{decode_url(_path)};
                    auto offset = static_cast<uint64_t>(std::stoul(_offset));
                    auto limit  = static_cast<uint64_t>(std::stoul(_limit));

                    char read_buff[limit+1];
                    memset(&read_buff, 0, limit+1);

                    io::client::basic_transport<char> xport(*conn());

                    io::odstream ds{xport, path};
                    if(ds.is_open()) {
                        if(offset > 0) {
                            ds.seekp(offset);
                        }

                        auto write_size = std::min(_body.size(), limit);
                        ds.write(_body.c_str(), write_size);

                        return std::forward_as_tuple(
                                   Pistache::Http::Code::Ok,
                                   SUCCESS);
                    }

                    auto error = make_error(
                                     SYS_INVALID_INPUT_PARAM,
                                     fmt::format(
                                           "Failed to open object [{}]"
                                         , path.string()));
                    return std::forward_as_tuple(
                               Pistache::Http::Code::Bad_Request,
                               error);
                }
                catch(const irods::exception& _e) {
                    auto error = make_error(_e.code(), _e.what());
                    return std::forward_as_tuple(
                               Pistache::Http::Code::Bad_Request,
                               error);
                }

            } // operator()


    }; // class stream

}; // namespace irods::rest
