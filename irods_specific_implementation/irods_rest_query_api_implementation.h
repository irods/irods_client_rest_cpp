#ifndef IRODS_REST_CPP_QUERY_API_IMPLEMENTATION_H
#define IRODS_REST_CPP_QUERY_API_IMPLEMENTATION_H

#include "irods_rest_api_base.h"

#include "constants.hpp"
#include <irods/irods_query.hpp>
#include <irods/rodsGenQuery.h>
#include <irods/rodsErrorTable.h>

#include <algorithm>

#include <pistache/router.h>

namespace irods::rest
{
    // this is contractually tied directly to the api implementation
    const std::string service_name{"irods_rest_cpp_query_server"};

    class query : public api_base
    {
    public:
        query()
            : api_base{service_name}
        {
            info("Endpoint initialized.");
        }

        std::tuple<Pistache::Http::Code, std::string>
        operator()(const Pistache::Rest::Request& _request,
                   Pistache::Http::ResponseWriter& _response)
        {
            try {
                auto _query_string = _request.query().get("query").get();
                auto _query_limit = _request.query().get("limit").get();
                auto _row_offset = _request.query().get("offset").get();
                auto _query_type = _request.query().get("type").get();
                auto _case_sensitive = _request.query().get("case-sensitive").getOrElse("1");
                auto _distinct = _request.query().get("distinct").getOrElse("1");

                auto conn = get_connection(_request.headers().getRaw("authorization").value(), _query_string);

                std::string query_string{decode_url(_query_string)};
                if ("0" == _case_sensitive) {
                    std::transform(query_string.begin(), query_string.end(), query_string.begin(), [](unsigned char c) {
                        return std::toupper(c);
                    });
                }

                uintmax_t row_offset  = std::stoi(_row_offset);
                uintmax_t query_limit = std::stoi(_query_limit);

                const auto query_type = irods::query<rcComm_t>::convert_string_to_query_type(_query_type);
                const auto options = init_query_options(_case_sensitive, _distinct);

                irods::query query{conn(), query_string, query_limit, row_offset, query_type, options};

                uintmax_t current_row_count = 0;
                uintmax_t total_row_count = query.size();
                nlohmann::json arrays = nlohmann::json::array();
                for (auto&& row : query) {
                    ++current_row_count;

                    nlohmann::json array = nlohmann::json::array();
                    for (auto&& e : row) {
                        array.push_back(e);
                    }

                    arrays.push_back(array);
                }

                total_row_count += row_offset;

                nlohmann::json results = nlohmann::json::object();
                results["_embedded"] = arrays;
                results["count"] = std::to_string(current_row_count);
                results["total"] = std::to_string(total_row_count);

                double dbl_row_offset  = static_cast<double>(row_offset);
                double dbl_query_limit = static_cast<double>(query_limit);
                double dbl_total_row_count = static_cast<double>(total_row_count);
                nlohmann::json links = nlohmann::json::object();
                constexpr auto* url_part = "query?query={}&limit={}&offset={}&type={}&case-sensitive={}&distinct={}";
                links["self"] = base_url + fmt::format(url_part,
                                            query_string,
                                            _query_limit,
                                            _row_offset,
                                            _query_type,
                                            _case_sensitive,
                                            _distinct);

                links["first"] = base_url + fmt::format(url_part,
                                             query_string,
                                             _query_limit,
                                             "0",
                                             _query_type,
                                             _case_sensitive,
                                             _distinct);

                double total_pages = dbl_total_row_count / dbl_query_limit;
                double fraction_remaining_pages = total_pages - std::trunc(total_pages);
                double remaining_rows = fraction_remaining_pages * static_cast<double>(dbl_query_limit);
                double final_page_delta = (remaining_rows == 0.0) ? dbl_query_limit : remaining_rows;
                double last_page_number = dbl_total_row_count - final_page_delta;

                links["last"] = base_url + fmt::format(url_part,
                                            query_string,
                                            _query_limit,
                                            static_cast<int>(last_page_number),
                                            _query_type,
                                            _case_sensitive,
                                            _distinct);

                double current_page_number = dbl_row_offset / dbl_query_limit;
                double next_page_number = std::trunc(current_page_number) + 1 * dbl_query_limit;
                next_page_number = (next_page_number >= dbl_total_row_count) ? last_page_number : next_page_number;

                links["next"] = base_url + fmt::format(url_part,
                                            query_string,
                                            _query_limit,
                                            static_cast<int>(next_page_number),
                                            _query_type,
                                            _case_sensitive,
                                            _distinct);

                auto prev_count = dbl_row_offset - dbl_query_limit;
                links["prev"] = base_url + fmt::format(url_part,
                                            query_string,
                                            _query_limit,
                                            static_cast<int>(std::max(0.0, prev_count)),
                                            _query_type,
                                            _case_sensitive,
                                            _distinct);
                results["_links"] = links;

                return std::make_tuple(Pistache::Http::Code::Ok, results.dump());
            }
            catch (const irods::exception& e) {
                error("Caught exception - [error_code={}] {}", e.code(), e.what());
                return make_error_response(e.code(), e.client_display_what());
            }
            catch (const std::exception& e) {
                error("Caught exception - {}", e.what());
                return make_error_response(SYS_INVALID_INPUT_PARAM, e.what());
            }
        } // operator()

    private:
        int init_query_options(const std::string_view _case_sensitive,
                               const std::string_view _distinct)
        {
            int options = 0;

            if (_case_sensitive == "0") {
                options |= UPPER_CASE_WHERE;
            }
            else if (_case_sensitive != "1") {
                THROW(SYS_INVALID_INPUT_PARAM, "Invalid query option: case_sensitive must be 0 or 1.");
            }

            if (_distinct == "0") {
                options |= NO_DISTINCT;
            }
            else if (_distinct != "1") {
                THROW(SYS_INVALID_INPUT_PARAM, "Invalid query option: distinct must be 0 or 1.");
            }

            return options;
        } // init_query_options
    }; // class query
} // namespace irods::rest

#endif // IRODS_REST_CPP_QUERY_API_IMPLEMENTATION_H

