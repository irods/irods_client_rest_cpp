
#include "irods_rest_api_base.h"
#include "zone_report.h"

// this is contractually tied directly to the swagger api definition, and the below implementation
#define MACRO_IRODS_ZONE_REPORT_API_IMPLEMENTATION \
    Pistache::Http::Code code; \
    std::string message; \
    irods_zone_report_.add_headers(response); \
    std::tie(code, message) = irods_zone_report_(headers.getRaw("Authorization").value()); \
    response.send(code, message);

namespace irods::rest {
    class zone_report : public api_base {
        public:
        std::tuple<Pistache::Http::Code &&, std::string> operator()(
            const std::string& _auth_header)
        {
            std::string jwt;
            try {
                auto conn = get_connection(_auth_header);

                bytesBuf_t* bbuf = nullptr;
                auto err = rcZoneReport(conn(), &bbuf);
                if(err < 0) {
                    std::string report = rodsErrorName(err, nullptr);
                    return std::forward_as_tuple(
                               Pistache::Http::Code::Bad_Request,
                               report);
                }

                auto report = std::string{static_cast<char*>(bbuf->buf),
                                          static_cast<char*>(bbuf->buf)+bbuf->len};

                free(bbuf);

                return std::forward_as_tuple(
                           Pistache::Http::Code::Ok,
                           report);
            }
            catch(const irods::exception& _e) {
                return std::forward_as_tuple(
                           Pistache::Http::Code::Bad_Request, _e.what());
            }

        } // operator()
    }; // class zone_report
}; // namespace irods::rest
