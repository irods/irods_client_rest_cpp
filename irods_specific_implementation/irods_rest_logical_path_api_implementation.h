#include "irods_rest_api_base.h"
#include "constants.hpp"
#include "utils.hpp"

#include <irods/filesystem.hpp>
#include <irods/rodsClient.h>
#include <irods/irods_random.hpp>
#include <irods/rodsErrorTable.h>
#include <irods/irods_at_scope_exit.hpp>
#include <irods/irods_query.hpp>
#include <irods/key_value_proxy.hpp>

#include <pistache/http_headers.h>
#include <pistache/optional.h>
#include <pistache/router.h>

namespace fs = irods::experimental::filesystem;
namespace fscli = irods::experimental::filesystem::client;

namespace irods::rest
{
    const std::string service_name{"irods_rest_cpp_logicalpath_server"};

    // this is contractually tied directly to the api implementation
    class logical_path : public api_base
    {
    public:
        logical_path()
            : api_base{service_name}
        {
            info("Endpoint initialized.");
        }

        std::tuple<Pistache::Http::Code, std::string> rename_dispatcher(const Pistache::Rest::Request& _request,
                                                                        Pistache::Http::ResponseWriter& _response)
        {
            namespace fs = irods::experimental::filesystem;

            try {
                auto _src = _request.query().get("src").get();
                auto _dst = _request.query().get("dst").get();

                const fs::path src = decode_url(_src);
                const fs::path dst = decode_url(_dst);

                auto conn = get_connection(_request.headers().getRaw("authorization").value());

                fs::client::rename(*conn(), src, dst);

                return std::make_tuple(Pistache::Http::Code::Ok, "");
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
                return make_error_response(SYS_INTERNAL_ERR, e.what());
            }
            catch (...) {
                error("Unknown error occurred");
                return make_error_response(SYS_UNKNOWN_ERROR, "Unknown error occurred");
            }
        }

        std::tuple<Pistache::Http::Code, std::string> delete_dispatcher(const Pistache::Rest::Request& _request,
                                                                        Pistache::Http::ResponseWriter& _response)
        {
            namespace fs = irods::experimental::filesystem;
            namespace fscli = irods::experimental::filesystem::client;

            try {
                const auto _logical_path = _request.query().get("logical-path").get();
                const auto _no_trash = _request.query().get("no-trash").getOrElse("0");
                const auto _recursive = _request.query().get("recursive").getOrElse("0");
                const auto _unregister = _request.query().get("unregister").getOrElse("0");

                const bool recursive = ("1" == _recursive);

                auto conn = get_connection(_request.headers().getRaw("authorization").value());

                const fs::path logical_path = decode_url(_logical_path);

                if (!recursive && fscli::is_collection(fscli::status(*conn(), logical_path))) {
                    return make_error_response(USER_INCOMPATIBLE_PARAMS,
                                               "'recursive=1' required to delete a collection. Make sure you want to "
                                               "delete the whole sub-tree.");
                }

                fs::extended_remove_options opts{.no_trash = "1" == _no_trash,
                                                 .verbose = false,
                                                 .progress = false,
                                                 .recursive = recursive,
                                                 .unregister = "1" == _unregister};

                bool status = fs::client::remove(*conn(), logical_path, opts);

                if (!status) {
                    return make_error_response(
                        SYS_UNKNOWN_ERROR,
                        "Client experienced an unknown error while processing your delete request.");
                }

                return std::make_tuple(Pistache::Http::Code::Ok, "");
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
                return make_error_response(SYS_INTERNAL_ERR, e.what());
            }
            catch (...) {
                error("Caught unknown exception.");
                return make_error_response(SYS_UNKNOWN_ERROR, "Caught unknown exception");
            }
        }

        auto trim_dispatcher(const Pistache::Rest::Request& _request, Pistache::Http::ResponseWriter& _response)
            -> std::tuple<Pistache::Http::Code, std::string>
        {
            try {
                auto conn = get_connection(_request.headers().getRaw("authorization").value());

                auto inp = create_data_obj_trim_inp(_request);

                // This will ensure that the KeyValPair member of the input is free'd.
                const auto trim_input_lm = irods::at_scope_exit{[&inp] { clearKeyVal(&inp.condInput); }};

                if (fscli::is_collection(fscli::status(*conn(), decode_url(inp.objPath)))) {
                    if (!is_set(_request.query().get("recursive").getOrElse("0"))) {
                        return make_error_response(
                            USER_INCOMPATIBLE_PARAMS,
                            "'recursive=1' required to trim a collection. Make sure you want to trim the whole "
                            "sub-tree.");
                    }

                    for (const auto& pathname : fscli::recursive_collection_iterator(*conn(), inp.objPath)) {
                        if (fscli::is_collection(fscli::status(*conn(), decode_url(pathname.path())))) {
                            continue;
                        }

                        std::strncpy(inp.objPath, pathname.path().c_str(), MAX_NAME_LEN);

                        if (const auto ec = rcDataObjTrim(conn(), &inp); ec < 0) {
                            const auto msg = fmt::format("Error occurred during trim of [{}]", inp.objPath);
                            return make_error_response(ec, msg);
                        }
                    }

                    return std::make_tuple(Pistache::Http::Code::Ok, "");
                }

                if (const auto ec = rcDataObjTrim(conn(), &inp); ec < 0) {
                    const auto msg = fmt::format("Error occurred during trim of [{}]", inp.objPath);
                    return make_error_response(ec, msg);
                }

                return std::make_tuple(Pistache::Http::Code::Ok, "");
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
            catch (...) {
                error("Caught unknown exception.");
                return make_error_response(SYS_UNKNOWN_ERROR, "Caught unknown exception");
            }
        } // trim_dispatcher

        auto replicate_dispatcher(const Pistache::Rest::Request& _request, Pistache::Http::ResponseWriter& _response)
            -> std::tuple<Pistache::Http::Code, std::string>
        {
            try {
                auto conn = get_connection(_request.headers().getRaw("authorization").value());

                auto inp = create_data_obj_repl_inp(_request);

                // This will ensure that the KeyValPair member of the input is free'd.
                const auto trim_input_lm = irods::at_scope_exit{[&inp] { clearKeyVal(&inp.condInput); }};

                if (fscli::is_collection(fscli::status(*conn(), decode_url(inp.objPath)))) {
                    if (!is_set(_request.query().get("recursive").getOrElse("0"))) {
                        return std::make_tuple(
                            Pistache::Http::Code::Bad_Request,
                            "'recursive=1' required to replicate a collection. Make sure you want to replicate the "
                            "whole sub-tree.");
                    }

                    for (const auto& pathname : fscli::recursive_collection_iterator(*conn(), inp.objPath)) {
                        if (fscli::is_collection(fscli::status(*conn(), decode_url(pathname.path())))) {
                            continue;
                        }

                        std::strncpy(inp.objPath, pathname.path().c_str(), MAX_NAME_LEN);

                        if (const auto ec = rcDataObjRepl(conn(), &inp); ec != 0) {
                            const auto msg = fmt::format("Error occurred during replication of [{}]", inp.objPath);
                            return make_error_response(ec, msg);
                        }
                    }

                    return std::make_tuple(Pistache::Http::Code::Ok, "");
                }

                if (const auto ec = rcDataObjRepl(conn(), &inp); ec != 0) {
                    const auto msg = fmt::format("Error occurred during replication of [{}]", inp.objPath);
                    return make_error_response(ec, msg);
                }

                return std::make_tuple(Pistache::Http::Code::Ok, "");
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
            catch (...) {
                error("Caught unknown exception.");
                return make_error_response(SYS_UNKNOWN_ERROR, "Caught unknown exception");
            }
        } // replicate_dispatcher

      private:
        static auto create_data_obj_trim_inp(const Pistache::Http::Request& _request) -> DataObjInp
        {
            const auto _logical_path = _request.query().get("logical-path").getOrElse("");
            if (_logical_path.empty()) {
                throw std::runtime_error{"logical-path missing from request or empty"};
            }

            DataObjInp inp{};
            std::strncpy(inp.objPath, _logical_path.c_str(), MAX_NAME_LEN);

            auto kvp = irods::experimental::key_value_proxy{inp.condInput};

            if (is_set(_request.query().get("recursive").getOrElse("0"))) {
                kvp[RECURSIVE_OPR__KW] = "";
            }

            if (is_set(_request.query().get("admin-mode").getOrElse("0"))) {
                kvp[ADMIN_KW] = "";
            }

            const auto _num = _request.query().get("minimum-number-of-remaining-replicas").getOrElse("");
            if (!_num.empty()) {
                kvp[COPIES_KW] = _num.c_str();
            }

            const auto _age = _request.query().get("minimum-age-in-minutes").getOrElse("");
            if (!_age.empty()) {
                kvp[AGE_KW] = _age.c_str();
            }

            const auto _replica_number = _request.query().get("replica-number").getOrElse("");
            if (!_replica_number.empty()) {
                kvp[REPL_NUM_KW] = _replica_number.c_str();
            }

            const auto _src_resc = _request.query().get("src-resource").getOrElse("");
            if (!_src_resc.empty()) {
                kvp[RESC_NAME_KW] = _src_resc.c_str();
            }

            return inp;
        } // create_data_obj_trim_inp

        static auto create_data_obj_repl_inp(const Pistache::Http::Request& _request) -> DataObjInp
        {
            const auto _logical_path = _request.query().get("logical-path").getOrElse("");
            if (_logical_path.empty()) {
                throw std::runtime_error{"logical-path missing from request or empty"};
            }

            DataObjInp inp{};
            std::strncpy(inp.objPath, _logical_path.c_str(), MAX_NAME_LEN);

            auto kvp = irods::experimental::key_value_proxy{inp.condInput};

            const auto _replica_number = _request.query().get("replica-number").getOrElse("");
            if (!_replica_number.empty()) {
                kvp[REPL_NUM_KW] = _replica_number.c_str();
            }

            const auto _thread_count = _request.query().get("thread-count").getOrElse("");
            if (!_thread_count.empty()) {
                inp.numThreads = std::stoi(_thread_count);
            }
            else {
                inp.numThreads = NO_THREADING;
            }

            const auto _src_resource = _request.query().get("src-resource").getOrElse("");
            if (!_src_resource.empty()) {
                kvp[RESC_NAME_KW] = _src_resource.c_str();
            }

            const auto _dst_resource = _request.query().get("dst-resource").getOrElse("");
            if (!_dst_resource.empty()) {
                kvp[DEST_RESC_NAME_KW] = _dst_resource.c_str();
            }
            else {
                const auto& cfg = irods::rest::configuration::irods_client_environment();
                kvp[DEST_RESC_NAME_KW] = cfg.at("default_resource").get_ref<const std::string&>();
            }

            if (is_set(_request.query().get("recursive").getOrElse("0"))) {
                kvp[RECURSIVE_OPR__KW] = "";
            }

            if (is_set(_request.query().get("admin-mode").getOrElse("0"))) {
                kvp[ADMIN_KW] = "";
            }

            if (is_set(_request.query().get("all").getOrElse("0"))) {
                kvp[ALL_KW] = "";
            }

            return inp;
        } // create_data_obj_repl_inp
    }; // class logical_path_rename
} // namespace irods::rest
