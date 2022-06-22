#include "irods_rest_api_base.h"
#include "constants.hpp"
#include "utils.hpp"

#include <irods/filesystem.hpp>
#include <irods/rodsClient.h>
#include <irods/irods_random.hpp>
#include <irods/rodsErrorTable.h>
#include <irods/irods_at_scope_exit.hpp>
#include <irods/irods_query.hpp>

#include <pistache/http_headers.h>
#include <pistache/optional.h>
#include <pistache/router.h>

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
                error("Caught unknown exception");
                return make_error_response(SYS_UNKNOWN_ERROR, "Unknown error occurred");
            }
        }
    }; // class logical_path_rename
} // namespace irods::rest
