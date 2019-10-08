
#include "irods_rest_api_base.h"

#include "irods_query.hpp"

// this is contractually tied directly to the swagger api definition, and the below implementation
#define MACRO_IRODS_QUERY_API_IMPLEMENTATION \
    Pistache::Http::Code code; \
    std::string message; \
    std::tie(code, message) = irods_query_(headers.getRaw("Authorization").value(), queryString.get(), queryLimit.get(), rowOffset.get(), queryType.get()); \
    response.send(code, message);

namespace irods {
namespace rest {
class query : api_base {
    public:
    std::tuple<Pistache::Http::Code &&, std::string> operator()(
        const std::string& _auth_header,
        const std::string& _query_string,
        const std::string& _query_limit,
        const std::string& _row_offset,
        const std::string& _query_type) {

        auto conn = get_connection();
        authenticate(conn(), _auth_header);

        try {
            std::string query_string{decode_url(_query_string)};

            uintmax_t row_offset  = std::stoi(_row_offset);
            uintmax_t query_limit = std::stoi(_query_limit);

            auto query_type = irods::query<rcComm_t>::convert_string_to_query_type(_query_type);
            irods::query<rcComm_t> qobj{conn(), query_string, query_limit, row_offset, query_type};

            nlohmann::json arrays = nlohmann::json::array();
            for(auto row : qobj) {
                nlohmann::json array = nlohmann::json::array();
                for(auto e : row) {
                    array += e;
                }

                arrays += array;
            }

            nlohmann::json results = nlohmann::json::object();
            results["_embedded"] = arrays;

            nlohmann::json links = nlohmann::json::object();
            std::string base_url{"query?query_string=%s&query_limit=%s&row_offset=%s&query_type=%s"};
            links["self"] = boost::str(boost::format(base_url)
                            % _query_string
                            % _query_limit
                            % _row_offset
                            % _query_type);
            links["first"] = boost::str(boost::format(base_url)
                            % _query_string
                            % _query_limit
                            % "0"
                            % _query_type);
            links["last"] = boost::str(boost::format(base_url)
                            % _query_string
                            % _query_limit
                            % "FIXME"
                            % _query_type);
            links["next"] = boost::str(boost::format(base_url)
                            % _query_string
                            % _query_limit
                            % std::to_string(row_offset+query_limit)
                            % _query_type);
            links["prev"] = boost::str(boost::format(base_url)
                            % _query_string
                            % _query_limit
                            % std::to_string(std::max((uintmax_t)0, row_offset-query_limit))
                            % _query_type);
            results["_links"] = links;

            return std::forward_as_tuple(
                    Pistache::Http::Code::Ok,
                    results.dump());
        }
        catch(const irods::exception& _e) {
            return std::forward_as_tuple(
                    Pistache::Http::Code::Bad_Request,
                    _e.what());
        }
    } // operator()
}; // class query
}; // namespace rest
}; // namespace irods
