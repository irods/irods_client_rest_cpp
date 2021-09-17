#ifndef IRODS_REST_CPP_LIST_API_IMPLEMENTATION_H
#define IRODS_REST_CPP_LIST_API_IMPLEMENTATION_H

#include "irods_rest_api_base.h"

#include "filesystem.hpp"
#include "rodsErrorTable.h"

#include <fstream>

// this is contractually tied directly to the swagger api definition, and the below implementation
#define MACRO_IRODS_LIST_API_IMPLEMENTATION \
    Pistache::Http::Code code; \
    std::string message; \
    std::tie(code, message) = irods_list_(headers.getRaw("authorization").value(), path.get(), stat.get(), permissions.get(), metadata.get(), offset.get(), limit.get(), base); \
    response.send(code, message);

namespace irods::rest {

    // this is contractually tied directly to the api implementation
    const std::string service_name{"irods_rest_cpp_list_server"};

    namespace fs   = irods::experimental::filesystem;
    namespace fcli = irods::experimental::filesystem::client;
    using     fsp  = fs::path;

    class list : public api_base
    {
    public:
        list()
            : api_base{service_name}
        {
            trace("Endpoint initialized.");
        }

        std::tuple<Pistache::Http::Code, std::string>
        operator()(const std::string& _auth_header,
                   const std::string& _logical_path,
                   const std::string& _stat,
                   const std::string& _permissions,
                   const std::string& _metadata,
                   const std::string& _offset,
                   const std::string& _limit,
                   const std::string& _base_url)
        {
            trace("Handling request ...");

            info("Input arguments - path=[{}], stat=[{}], permissions=[{}], metadata=[{}], "
                 "offset=[{}], limit=[{}]",
                 _logical_path, _stat, _permissions, _metadata, _offset, _limit);

            try {
                auto conn = get_connection(_auth_header);

                std::string logical_path{decode_url(_logical_path)};
                intmax_t limit_counter{0};
                intmax_t offset_counter{0};
                const intmax_t offset = std::stoi(_offset);
                const intmax_t limit  = std::stoi(_limit);

                const bool stat        = ("true" == _stat);
                const bool permissions = ("true" == _permissions);
                const bool metadata    = ("true" == _metadata);

                nlohmann::json objects = nlohmann::json::array();

                fsp start_path{logical_path};

                if (fcli::is_data_object(*conn(), start_path)) {
                    const auto object_status = fcli::status(*conn(), start_path);

                    nlohmann::json obj_info = nlohmann::json::object();
                    obj_info["type"] = type_to_string.at(object_status.type());
                    obj_info["logical_path"] = start_path.string();

                    if (stat) { aggregate_stat_information(*conn(), obj_info, start_path); }
                    if (permissions) { aggregate_permissions_information(obj_info, object_status.permissions()); }
                    if (metadata) { aggregate_metadata_information(*conn(), obj_info, start_path); }

                    objects.push_back(obj_info);
                }
                else if (fcli::is_collection(*conn(), start_path)) {
                    for (auto&& p : fcli::recursive_collection_iterator(*conn(), start_path)) {
                        // skip earlier entries for paging
                        if (offset > 0 && offset_counter < offset) {
                            ++offset_counter;
                            continue;
                        }

                        try {
                            const auto object_status = fcli::status(*conn(), p.path());

                            nlohmann::json obj_info = nlohmann::json::object();
                            obj_info["type"] = type_to_string.at(object_status.type());
                            obj_info["logical_path"] = p.path().c_str();

                            if (stat) { aggregate_stat_information(*conn(), obj_info, p.path()); }
                            if (permissions) { aggregate_permissions_information(obj_info, object_status.permissions()); }
                            if (metadata) { aggregate_metadata_information(*conn(), obj_info, p.path()); }

                            objects.push_back(obj_info);
                        }
                        catch (const irods::exception& e) {
                            error("Caught exception - [error_code={}] {}", e.code(), e.what());
                            return make_error_response(e.code(), e.client_display_what());
                        }

                        ++limit_counter;
                        if (limit > 0 && limit_counter >= limit) {
                            break;
                        }
                    } // for path
                }
                else {
                    const auto msg = fmt::format("Logical path [{}] is not accessible.", logical_path);
                    error(msg);
                    return make_error_response(SYS_INVALID_INPUT_PARAM, msg);
                }

                nlohmann::json results = nlohmann::json::object();
                results["_embedded"] = objects;

                nlohmann::json links = nlohmann::json::object();
                const std::string base_url = _base_url + "/list?path={}&stat={}&permissions={}&metadata={}&offset={}&limit={}";
                links["self"] = fmt::format(base_url
                                , _logical_path
                                , _stat
                                , _permissions
                                , _metadata
                                , _offset
                                , _limit);
                links["first"] = fmt::format(base_url
                                , _logical_path
                                , _stat
                                , _permissions
                                , _metadata
                                , "0"
                                , _limit);
                links["last"] = fmt::format(base_url
                                , _logical_path
                                , _stat
                                , _permissions
                                , _metadata
                                , "UNSUPPORTED"
                                , _limit);
                links["next"] = fmt::format(base_url
                                , _logical_path
                                , _stat
                                , _permissions
                                , _metadata
                                , std::to_string(offset+limit)
                                , _limit);
                links["prev"] = fmt::format(base_url
                                , _logical_path
                                , _stat
                                , _permissions
                                , _metadata
                                , std::to_string(std::max((intmax_t) 0, offset - limit))
                                , _limit);

                results["_links"] = links;

                return std::make_tuple(Pistache::Http::Code::Ok, results.dump());
            }
            catch (const fs::filesystem_error& e) {
                error("Caught exception - [error_code={}] {}", e.code().value(), e.what());
                return make_error_response(e.code().value(), e.what());
            }
            catch (const irods::exception& e) {
                error("Caught exception - [error_code={}] {}", e.code(), e.what());
                return make_error_response(e.code(), e.what());
            }
            catch (const std::exception& e) {
                error("Caught exception - {}", e.what());
                return make_error_response(SYS_INVALID_INPUT_PARAM, e.what());
            }
        } // operator()

    private:
        const std::map<irods::experimental::filesystem::perms, std::string> perm_to_string = {
            {irods::experimental::filesystem::perms::null,  "null"},
            {irods::experimental::filesystem::perms::read,  "read"},
            {irods::experimental::filesystem::perms::write, "write"},
            {irods::experimental::filesystem::perms::own,   "own"}
         };

        const std::map<irods::experimental::filesystem::object_type, std::string> type_to_string = {
            {irods::experimental::filesystem::object_type::none,               "none"},
            {irods::experimental::filesystem::object_type::not_found,          "not_found"},
            {irods::experimental::filesystem::object_type::data_object,        "data_object"},
            {irods::experimental::filesystem::object_type::collection,         "collection"},
            {irods::experimental::filesystem::object_type::special_collection, "special_collection"},
            {irods::experimental::filesystem::object_type::unknown,            "unknown"},
        };

        void aggregate_stat_information(rcComm_t& _comm,
                                        nlohmann::json& _obj_info,
                                        const fs::path& _path)
        {
            auto stat_info = nlohmann::json::object();

            stat_info["size"] = fcli::data_object_size(_comm, _path);

            using clock_type = std::chrono::system_clock;
            const auto last_write_time = clock_type::to_time_t(fcli::last_write_time(_comm, _path));
            stat_info["last_write_time"] = std::to_string(last_write_time);

            _obj_info["status_information"] = stat_info;
        } // aggregate_stat_information

        void aggregate_permissions_information(nlohmann::json& _obj_info,
                                               const std::vector<fs::entity_permission>& _perms)
        {
            auto perm_info = nlohmann::json::object();

            for (auto&& p : _perms) {
                perm_info[p.name] = perm_to_string.at(p.prms);
            }

            _obj_info["permission_information"] = perm_info;
        } // aggregate_permissions_information

        void aggregate_metadata_information(rcComm_t& _comm,
                                            nlohmann::json& _obj_info,
                                            const fs::path& _path)
        {
            auto meta_arr = nlohmann::json::array();

            for (auto&& avu : fcli::get_metadata(_comm, _path)) {
                auto md = nlohmann::json::object();
                md["attribute"] = avu.attribute;
                md["value"] = avu.value;
                md["units"] = avu.units;
                meta_arr.push_back(md);
            }

            _obj_info["metadata"] = meta_arr;
        } // aggregate_metadata_information
    }; // class list
} // namespace irods::rest

#endif // IRODS_REST_CPP_LIST_API_IMPLEMENTATION_H

