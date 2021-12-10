#ifndef IRODS_REST_CPP_UTILS_HPP
#define IRODS_REST_CPP_UTILS_HPP

#include "pistache/http.h"
#include "pistache/router.h"

#include <nlohmann/json.hpp>
#include "spdlog/spdlog.h"

namespace irods::rest
{
    template <typename ApiImpl>
    auto handle_request(ApiImpl& _api_impl,
                        const Pistache::Rest::Request& _request,
                        Pistache::Http::ResponseWriter& _response)
    {
        nlohmann::json request_info{
            {"remote_address", _request.address().host()},
            {"query", _request.query().as_str()},
            {"message", "Request initiated."}
        };

        spdlog::debug(request_info.dump());

        auto [http_code, msg] = _api_impl(_request, _response);

        request_info["status"] = http_code;
        request_info["message"] = "Request completed.";

        spdlog::info(request_info.dump());

        _response.send(http_code, msg);
    } // handle_request
} // namespace irods::rest

#endif // IRODS_REST_CPP_UTILS_HPP

