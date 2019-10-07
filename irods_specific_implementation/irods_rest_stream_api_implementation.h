
#include "irods_rest_api_base.h"

#include "dstream.hpp"
#include "transport/default_transport.hpp"

// this is contractually tied directly to the swagger api definition, and the below implementation
#define MACRO_IRODS_STREAM_API_IMPLEMENTATION \
    Pistache::Http::Code code; \
    std::string message; \
    std::tie(code, message) = irods_stream_(headers.getRaw("Authorization").value(), body, path.get(), offset.get(), limit.get()); \
    response.send(code, message);

namespace irods {
namespace rest {
class stream : api_base {
    public:
    std::tuple<Pistache::Http::Code &&, std::string> operator()(
        const std::string& _auth_header,
        const std::string& _body,
        const std::string& _path,
        const std::string& _offset,
        const std::string& _limit) {

        auto conn = get_connection();
        authenticate(conn(), _auth_header);

        try {
            uintmax_t offset = std::stoi(_offset);
            uintmax_t limit  = std::stoi(_limit);

            char read_buff[limit+1];
            memset(&read_buff, 0, limit+1);
            irods::experimental::io::client::basic_transport<char> xport(*conn());
            irods::experimental::io::dstream ds{xport, _path, std::ios_base::in | std::ios_base::out};
            if(offset > 0) {
                ds.seekg(offset);
            }

            if(ds.is_open()) {
                if(_body.empty()) {
                    ds.read(read_buff, limit);
                    return std::forward_as_tuple(
                               Pistache::Http::Code::Ok,
                               std::string{read_buff});
                }
                else {
                    auto write_size = _body.size() > limit ? limit : _body.size();
                    ds.write(_body.c_str(), write_size);
                    return std::forward_as_tuple(
                               Pistache::Http::Code::Ok, "Success");
                }
            }

            std::string msg{"Failed to open object ["};
            msg += _path;
            msg += "]";
            return std::forward_as_tuple(
                       Pistache::Http::Code::Bad_Request,
                       msg);
        }
        catch(const irods::exception& _e) {
            return std::forward_as_tuple(
                       Pistache::Http::Code::Bad_Request,
                       _e.what());
        }

    } // operator()


}; // class stream
}; // namespace rest
}; // namespace irods
