#ifndef IRODS_REST_CPP_UTILS_HPP
#define IRODS_REST_CPP_UTILS_HPP

#include <pistache/http.h>
#include <pistache/router.h>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>

namespace irods::rest
{
    class admin;

    auto hide_sensitive_data(const Pistache::Http::Uri::Query& _query,
                             nlohmann::json& _request_info) -> void
    {
        // Return immediately if "arg3=password" or "arg4" do not exist.
        if (_query.get("arg3").getOrElse("") != "password" || !_query.has("arg4")) {
            _request_info["query"] = _query.as_str();
            return;
        }

        auto safe_query = _query;
        safe_query.clear();

        // Build a new query string, but hide the password value (i.e. arg4) 
        // with a different value.
        std::for_each(_query.parameters_begin(), _query.parameters_end(),
            [&safe_query](const auto& _e) {
                safe_query.add(_e.first, (_e.first == "arg4") ? "[REDACTED]" : _e.second);
            });

        _request_info["query"] = safe_query.as_str();
    } // hide_sensitive_data

    template <typename ApiImpl>
    auto handle_request(ApiImpl& _api_impl,
                        const Pistache::Rest::Request& _request,
                        Pistache::Http::ResponseWriter& _response)
    {
        nlohmann::json request_info{
            {"remote_address", _request.address().host()},
            {"message", "Request initiated."}
        };

        if constexpr (std::is_same_v<ApiImpl, admin>) {
            hide_sensitive_data(_request.query(), request_info);
        }
        else {
            request_info["query"] = _request.query().as_str();
        }

        spdlog::debug(request_info.dump());

        auto [http_code, msg] = _api_impl(_request, _response);

        request_info["status"] = http_code;
        request_info["message"] = "Request completed.";

        spdlog::info(request_info.dump());

        _response.send(http_code, msg);
    } // handle_request
} // namespace irods::rest

#endif // IRODS_REST_CPP_UTILS_HPP

