#include "connection_pool.hpp"
#include "irods_query.hpp"
#include "boost/format.hpp"
#include "json.hpp"
#include "pistache/http_defs.h"
#include <tuple>

// this is contractually tied directly to the swagger api definition, and the below implementation
#define MACRO_IRODS_QUERY_API_IMPLEMENTATION \
    Pistache::Http::Code code; \
    std::string message; \
    std::tie(code, message) = irods_query_(queryString.get(), queryLimit.get(), rowOffset.get(), queryType.get()); \
    response.send(code, message);

namespace irods {
namespace rest {
class query {
    public:
    query() {
        rodsEnv env{};
            _getRodsEnv(env);
            connection_pool_ = std::make_shared<irods::connection_pool>(
                1,
                env.rodsHost,
                env.rodsPort,
                env.rodsUserName,
                env.rodsZone,
                env.irodsConnectionPoolRefreshTime);
    } // ctor

    std::tuple<Pistache::Http::Code &&, std::string> operator()(
        const std::string& _query_string,
        const std::string& _query_limit,
        const std::string& _row_offset,
        const std::string& _query_type) {
        auto conn = connection_pool_->get_connection();
        std::string query_string;
        if(url_decode(_query_string, query_string)) {
            try {
                uintmax_t row_offset  = std::stoi(_row_offset);
                uintmax_t query_limit = std::stoi(_query_limit);
                auto query_type  = irods::query<rcComm_t>::convert_string_to_query_type(_query_type);
                irods::query<rcComm_t> qobj{&static_cast<rcComm_t&>(conn), query_string, query_limit, row_offset, query_type};

                nlohmann::json results = nlohmann::json::object();
                nlohmann::json arrays = nlohmann::json::array();
                for(auto row : qobj) {
                    nlohmann::json array = nlohmann::json::array();
                    for(auto e : row) {
                        array += e;
                    }

                    arrays += array;
                }
                results["results"] = arrays;

                return std::forward_as_tuple(Pistache::Http::Code::Ok, results.dump());
            }
            catch(const irods::exception& _e) {
                return std::forward_as_tuple(Pistache::Http::Code::Bad_Request, _e.what());
            }
        } // url_decode

        return std::forward_as_tuple(Pistache::Http::Code::Bad_Request, "Invlaid URL Encoding");

    } // operator()

    private:
    std::shared_ptr<irods::connection_pool> connection_pool_;

    bool url_decode(const std::string& in, std::string& out)
    {
        out.clear();
        out.reserve(in.size());
        for (std::size_t i = 0; i < in.size(); ++i)
        {
            if (in[i] == '%')
            {
                if (i + 3 <= in.size())
                {
                    int value = 0;
                    std::istringstream is(in.substr(i + 1, 2));
                    if (is >> std::hex >> value)
                    {
                        out += static_cast<char>(value);
                        i += 2;
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    return false;
                }
            }
            else if (in[i] == '+')
            {
                out += ' ';
            }
            else
            {
                out += in[i];
            }
        }
        return true;
    }
}; // class query
}; // namespace rest
}; // namespace irods
