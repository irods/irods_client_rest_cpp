
#include "irods_rest_api_base.h"

#include "irods_query.hpp"
#include "irods_logger.hpp"

using logger = irods::experimental::log;

// this is contractually tied directly to the swagger api definition, and the below implementation
#define MACRO_IRODS_QUERY_API_IMPLEMENTATION \
    Pistache::Http::Code code; \
    std::string message; \
    std::tie(code, message) = irods_query_(headers.getRaw("Authorization").value(), queryString.get(), queryLimit.get(), rowOffset.get(), queryType.get(), base); \
    response.send(code, message);

namespace irods::rest {

    // this is contractually tied directly to the api implementation
    const std::string service_name{"irods_rest_cpp_query_server"};

    class query : public api_base {
        public:
            query() : api_base{service_name}
            {
                // ctor
            }

            std::tuple<Pistache::Http::Code &&, std::string> operator()(
                const std::string& _auth_header,
                const std::string& _query_string,
                const std::string& _query_limit,
                const std::string& _row_offset,
                const std::string& _query_type,
                const std::string& _base_url) {

                auto conn = get_connection(_auth_header);

                try {
                    std::string query_string{decode_url(_query_string)};

                    uintmax_t row_offset  = std::stoi(_row_offset);
                    uintmax_t query_limit = std::stoi(_query_limit);

                    auto query_type = irods::query<rcComm_t>::convert_string_to_query_type(_query_type);
                    irods::query<rcComm_t> qobj{conn(), query_string, query_limit, row_offset, query_type};

                    uintmax_t current_row_count{0};
                    uintmax_t total_row_count{qobj.size()};
                    nlohmann::json arrays = nlohmann::json::array();
                    for(auto row : qobj) {
                        ++current_row_count;
                        nlohmann::json array = nlohmann::json::array();
                        for(auto e : row) {
                            array += e;
                        }

                        arrays += array;
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
                    std::string base_url{_base_url+"query?query_string={}&query_limit={}&row_offset={}&query_type={}"};
                    links["self"] = fmt::format(base_url
                                    , query_string
                                    , _query_limit
                                    , _row_offset
                                    , _query_type);

                    links["first"] = fmt::format(base_url
                                    , query_string
                                    , _query_limit
                                    , "0"
                                    , _query_type);

                    double total_pages = dbl_total_row_count / dbl_query_limit;
                    double fraction_remaining_pages = (total_pages - std::trunc(total_pages));
                    double remaining_rows = fraction_remaining_pages * static_cast<double>(dbl_query_limit);
                    double final_page_delta{remaining_rows == 0.0 ? dbl_query_limit : remaining_rows};
                    double last_page_number{dbl_total_row_count - final_page_delta};

                    links["last"] = fmt::format(base_url
                                    , query_string
                                    , _query_limit
                                    , std::to_string(static_cast<int>(last_page_number))
                                    , _query_type);

                    double current_page_number = dbl_row_offset / dbl_query_limit;
                    double next_page_number{std::trunc(current_page_number)+1 * dbl_query_limit};
                    next_page_number = next_page_number >= dbl_total_row_count ? last_page_number : next_page_number;

                    links["next"] = fmt::format(base_url
                                    , query_string
                                    , _query_limit
                                    , std::to_string(static_cast<int>(next_page_number))
                                    , _query_type);

                    auto prev_count = dbl_row_offset - dbl_query_limit;
                    links["prev"] = fmt::format(base_url
                                    , query_string
                                    , _query_limit
                                    , std::to_string(static_cast<int>(std::max(0.0, prev_count)))
                                    , _query_type);
                    results["_links"] = links;

                    return std::forward_as_tuple(
                            Pistache::Http::Code::Ok,
                            results.dump());
                }
                catch(const irods::exception& _e) {
                    auto error = make_error(_e.code(), _e.what());
                    return std::forward_as_tuple(
                            Pistache::Http::Code::Bad_Request,
                            error);
                }
            } // operator()

    }; // class query

}; // namespace irods::rest
