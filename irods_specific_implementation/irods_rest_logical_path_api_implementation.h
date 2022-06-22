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
    const std::string service_name { "irods_rest_cpp_logicalpath_server" };

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
	    catch (...){
		error("Caught unknown exception.");
		return make_error_response(SYS_UNKNOWN_ERROR, "Caught unknown exception");
	    }
	}

        std::tuple<Pistache::Http::Code, std::string>
        trim_dispatcher(const Pistache::Rest::Request& _request,
                   Pistache::Http::ResponseWriter& _response)
        {
            try {
              auto conn = get_connection(_request.headers().getRaw("authorization").value());
              dataObjInp_t trim_input{};

              bool is_collection = false;

              setup_input_for_trim(
                _request,
                trim_input,
                *conn(),
                is_collection
              );

              int status;
              if ( is_collection ) {

                if ( ! is_set(_request.query().get("recursive").getOrElse("0")) ) {
                    return make_error_response(
                        USER_INCOMPATIBLE_PARAMS,
                        "'recursive=1' required to trim a collection. Make sure you want to trim the whole sub-tree."
                    );
                }

                for ( const auto& pathname : fscli::recursive_collection_iterator(*conn(), trim_input.objPath) ) {
                    if ( fscli::is_data_object( fscli::status( *conn(), decode_url(pathname.path()) ) ) ) {
                        strcpy(
                            trim_input.objPath,
                            pathname.path().c_str()
                        );

                        status = rcDataObjTrim(conn(), &trim_input);
                        if ( status == SYS_NOT_ALLOWED ) {
                            return make_error_response(status, "You have tried to trim a data object or collection you don't have permissions for.");
                        }
                        else if ( status == USER_INPUT_PATH_ERR ) {
                            return make_error_response(status, "You tried to trim a path that doesn't seem to exist in this zone.");
                        }
                        else if ( status == USER_INCOMPATIBLE_PARAMS ) {
                            return make_error_response(status, "You have specified two or more parameters for your trim operation which are incompatible with each other. See https://docs.irods.org/4.3.0/icommands/user/#itrim");
                        }
                        else if ( status == CAT_NO_ROWS_FOUND) {
                            return make_error_response(status, "Could not find the logical path you entered. You may have passed an empty string.");
                        }
                        else if ( status < 0 ) {
                            return make_error_response(status, "Client experienced an error processing your trim request. Please check the logs or notify your administrator.");
                        }
                    }
                }
              } else {
	    	if ( fscli::is_data_object( fscli::status( *conn(), decode_url(trim_input.objPath) ) ) ) {
		    status = rcDataObjTrim(conn(), &trim_input);
		} else {
		    return make_error_response(USER_INCOMPATIBLE_PARAMS, "Target is not a data object.");
		}
              }
              if ( status == SYS_NOT_ALLOWED ) {
                return  make_error_response(status, "You have tried to trim a data object or collection you don't have permissions for.");
              }
              else if ( status == USER_INPUT_PATH_ERR ) {
                return make_error_response(status, "You tried to trim a path that doesn't seem to exist in this zone.");
              }
              else if ( status == USER_INCOMPATIBLE_PARAMS ) {
                return make_error_response(status, "You have specified incompatible parameters for your trim operation.");
              }
	      else if ( status == CAT_NO_ROWS_FOUND) {
                return make_error_response(status, "Could not find the logical path you entered. You may have passed an empty string.");
              }
              else if ( status < 0 ) {
                return make_error_response(status, "Client experienced an error processing your trim request. Please check the logs or notify your administrator.");
              }

              return std::make_tuple(
                Pistache::Http::Code::Ok,
                ""
              );
            }
            catch (const irods::exception& e) {
                error("Caught exception - [error_code={}] {}", e.code(), e.what());
                return make_error_response(e.code(), e.what());
            }
            catch (const std::exception& e) {
                error("Caught exception - {}", e.what());
                return make_error_response(SYS_INVALID_INPUT_PARAM, e.what());
            }
	    catch (...){
		error("Caught unknown exception.");
		return make_error_response(SYS_UNKNOWN_ERROR, "Caught unknown exception");
	    }
        }

        std::tuple<Pistache::Http::Code, std::string>
        replicate_dispatcher(const Pistache::Rest::Request& _request,
                   Pistache::Http::ResponseWriter& _response)
        {
            try {
              dataObjInp_t repl_input{};

              auto conn = get_connection(_request.headers().getRaw("authorization").value());

              bool is_collection = false;

              setup_input_for_repl(
                  repl_input,
                  _request,
                  *conn(),
                  is_collection
              );

              int status;
              const auto start_path = repl_input.objPath;
              if ( is_collection ) {
                if ( ! is_set(_request.query().get("recursive").getOrElse("")) )
                {
                    return std::make_tuple(
                        Pistache::Http::Code::Bad_Request,
                        "'recursive=1' required to replicate a collection. Make sure you want to replicate the whole sub-tree."
                    );
                }

                for ( const auto& pathname : fscli::recursive_collection_iterator(*conn(), start_path) ) {
                    if ( fscli::is_data_object(fscli::status(*conn(), decode_url(pathname.path()))) ) {
			strcpy(
			  repl_input.objPath,
			  pathname.path().c_str()
			);

                        status = rcDataObjRepl(conn(), &repl_input);
                        if ( status == SYS_INVALID_FILE_PATH ) {
                            return make_error_response(status, "You passed an invalid path for replication. Make sure that you are giving an absolute path.");
                        }
                        else if ( status == SYS_NOT_ALLOWED ) {
                            std::stringstream msg;
                            msg << "You have tried to repl an object in a way that is not allowed.\n";
                            msg << "Some possible causes are:\n";
                            msg << "- You may have tried to replicate a non-good replica.\n";
                            msg << "- The object you tried to replicate may have a checksum that wasn't verified.\n";
                            msg << "- You may have tried to replicate a data object onto an already identical replica.\n";
                            msg << "Please check for these issues and contact your administrator if the issue isn't resolved.\n";
                            return make_error_response(SYS_NOT_ALLOWED, msg.str());
                        }
                        else if ( status == USER_INPUT_PATH_ERR ) {
                            return make_error_response(status, "You tried to trim a path that doesn't seem to exist in this zone.");
                        }
                        else if ( status == USER_INCOMPATIBLE_PARAMS ) {
                            return make_error_response(status, "You have specified two or more parameters for your trim operation which are incompatible with each other. See https://docs.irods.org/4.3.0/icommands/user/#itrim");
                        }
                        else if ( status < 0 ) {
                            return make_error_response(status, "Client experienced an error processing your trim request. Please check the logs or notify your administrator.");
                        }
                    }
                }
              } else {
                status = rcDataObjRepl(
                    conn(),
                    &repl_input
                );
                if ( status == SYS_INVALID_FILE_PATH ) {
                    return make_error_response(status, "You passed an invalid path for replication. Make sure that you are giving an absolute path.");
                }
                else if ( status == USER_INPUT_PATH_ERR ) {
                    return make_error_response(status, "You tried to trim a path that doesn't seem to exist in this zone.");
                }
                else if ( status == USER_INCOMPATIBLE_PARAMS ) {
                    return make_error_response(status, "You have specified incompatible parameters for your trim operation.");
                }
                else if ( status == SYS_NOT_ALLOWED ) {
                    std::stringstream msg;
                    msg << "You have tried to repl an object in a way that is not allowed.\n";
                    msg << "Some possible causes are:\n";
                    msg << "- You may have tried to replicate a non-good replica.\n";
                    msg << "- The object you tried to replicate may have a checksum that wasn't verified.\n";
                    msg << "- You may have tried to replicate a data object onto an already identical replica.\n";
                    msg << "Please check for these issues and contact your administrator if the issue isn't resolved.\n";
                    return make_error_response(SYS_NOT_ALLOWED, msg.str());
                }
                else if ( status < 0 ) {
                    return make_error_response(status, "Client experienced an error processing your trim request. Please check the logs or notify your administrator.");
                }
              }

                return std::make_tuple(
                    Pistache::Http::Code::Ok,
                    ""
                );
            }
	    catch (const fs::filesystem_error& e){
		error("Caught exception - [error_code=]", e.code().value());
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
	    catch (...){
		error("Caught unknown exception.");
		return make_error_response(SYS_UNKNOWN_ERROR, "Caught unknown exception");
	    }
        }

    private:
        void setup_input_for_trim(
                const Pistache::Http::Request& _request,
                dataObjInp_t& trim_input,
                rcComm_t& conn,
                bool& is_collection
            )
        {
              const auto _logical_path = _request.query().get("logical-path").getOrElse("");
              is_collection = fscli::is_collection(
                fscli::status(conn, decode_url(_logical_path))
              );

              // optional params
              // ---------------
              // Boolean
              const auto _recursive      = _request.query().get("recursive").getOrElse("0");
              // Boolean
              const auto _dryrun         = _request.query().get("dryrun").getOrElse("");
              // Integer
              const auto _age            = _request.query().get("minimum-age-in-minutes").getOrElse("");
              // Integer
              const auto _num            = _request.query().get("minimum-number-of-remaining-replicas").getOrElse("");
              // Integer
              const auto _replica_number = _request.query().get("replica-number").getOrElse("");
              // String
              const auto _src_resc       = _request.query().get("src-rescource").getOrElse("");
              // Boolean
              const auto _admin          = _request.query().get("admin-mode").getOrElse("");

	      // Set up key-value proxy
	      irods::experimental::key_value_proxy kvp {trim_input.condInput};

              if (is_set(_recursive)) {
		kvp[RECURSIVE_OPR__KW] = "";
              }

              if (is_set(_admin)) {
		kvp[ADMIN_KW] = "";
              }

              if (is_set(_dryrun)) {
		kvp[DRYRUN_KW] = "";
              }

              rstrcpy(
                trim_input.objPath,
                _logical_path.c_str(),
                MAX_NAME_LEN
              );

              if ( ! _num.empty() ) {
		  kvp[COPIES_KW] = _num.c_str();
              }

              if ( ! _age.empty() ) {
		  kvp[AGE_KW] = _age.c_str();
              }

              if ( ! _replica_number.empty() ) {
		  kvp[REPL_NUM_KW] = _replica_number.c_str();
              }

              if ( ! _src_resc.empty() ) {
		  kvp[RESC_NAME_KW] = _src_resc.c_str();
              }
        } // setup_input_for_trim

        void setup_input_for_repl(
            dataObjInp_t& _repl_input,
            const Pistache::Http::Request& _request,
            rcComm_t& _conn,
            bool& _is_collection
        ) {
              rodsEnv myRodsEnv;
              getRodsEnv(&myRodsEnv);

              // Optional parameters
              // Boolean parameters
              auto _all               = _request.query().get("all").getOrElse("0");
              auto _admin             = _request.query().get("admin-mode").getOrElse("0");
              auto _recursive         = _request.query().get("recursive").getOrElse("0");

              auto _src_resource      = _request.query().get("src-resource").getOrElse("");

              // Int parameters
              auto _thread_count       = _request.query().get("thread-count").getOrElse("");
              auto _replica_number          = _request.query().get("replica-number").getOrElse("");

              // String paramaeters
              auto _dst_resource     = _request.query().get("dst-resource").getOrElse("");

              // Required parameters
              auto _logical_path      = _request.query().get("logical-path").getOrElse("");
              rstrcpy(
                _repl_input.objPath,
                _logical_path.c_str(),
                MAX_NAME_LEN
              );

	      irods::experimental::key_value_proxy kvp {_repl_input.condInput};

              _is_collection = fscli::is_collection(
                fscli::status(_conn, decode_url(_logical_path))
              );

              if ( ! _replica_number.empty() ) {
		kvp[REPL_NUM_KW] = _replica_number.c_str();
              }

              if ( ! _thread_count.empty() ) {
                _repl_input.numThreads = std::stoi(_thread_count);
              } else {
                _repl_input.numThreads = NO_THREADING;
              }

              if ( ! _dst_resource.empty() ) {
		kvp[DEST_RESC_NAME_KW] = _dst_resource.c_str();
              } else if (strlen( myRodsEnv.rodsDefResource ) > 0) {
		kvp[DEST_RESC_NAME_KW] = myRodsEnv.rodsDefResource;
              }

              if ( is_set(_recursive) ) {
		  kvp[RECURSIVE_OPR__KW] = "";
              }

              if ( is_set(_admin) ) {
		kvp[ADMIN_KW] = "";
              }

              if ( is_set(_all) ) {
		kvp[ALL_KW] = "";
              }

              char resc[MAX_NAME_LEN] = { '\0' };
              if ( ! _dst_resource.empty() ) {
                  strcpy(
                    resc,
                    _dst_resource.c_str()
                  );
              }

              // If we didn't get a dest resource from the user,
              // get it from the rods environment
              if ( '\0' == resc[0] && strlen(myRodsEnv.rodsDefResource) > 0 ) {
                  strcpy(
                    resc,
                    myRodsEnv.rodsDefResource
                  );
              }

	      kvp[DEST_RESC_NAME_KW] = resc;
        } // setup_args_for_repl
    }; // class logical_path_rename
} // namespace irods::rest
