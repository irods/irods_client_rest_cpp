
#include "irods_rest_api_base.h"
#include "generalAdmin.h"

// this is contractually tied directly to the swagger api definition, and the below implementation
#define MACRO_IRODS_ADMIN_API_IMPLEMENTATION \
    Pistache::Http::Code code; \
    std::string message; \
    std::tie(code, message) = irods_admin_(headers.getRaw("Authorization").value(), action.get(), target.get(), arg2.get(), arg3.get(), arg4.get(), arg5.get(), arg6.get(), arg7.get()); \
    response.send(code, message);

namespace irods::rest {

    // this is contractually tied directly to the api implementation
    const std::string service_name{"irods_rest_cpp_admin_server"};

    class admin : public api_base {
        public:

            admin() : api_base{service_name}
            {
                // ctor
            }

            std::tuple<Pistache::Http::Code &&, std::string> operator()(
                const std::string& _auth_header,
                const std::string& _action,
                const std::string& _target,
                const std::string& _arg2,
                const std::string& _arg3,
                const std::string& _arg4,
                const std::string& _arg5,
                const std::string& _arg6,
                const std::string& _arg7)
            {

                auto conn = get_connection(_auth_header);

                try {
                    auto gen_inp = generalAdminInp_t{
                                       _action == "remove" ? "rm" : _action.c_str(),
                                       _target.c_str(),
                                       _arg2.c_str(),
                                       _arg3.c_str(),
                                       _arg4.c_str(),
                                       _arg5.c_str(),
                                       _arg6.c_str(),
                                       _arg7.c_str(),
                                       nullptr,
                                       nullptr};
                    auto err = rcGeneralAdmin(conn(), &gen_inp);
                    if(err < 0) {
                        auto error = make_error(err, "rsGeneralAdmin failed.");
                        return std::forward_as_tuple(
                                Pistache::Http::Code::Bad_Request,
                                error);
                    }

                    return std::forward_as_tuple(
                            Pistache::Http::Code::Ok,
                            SUCCESS);
                }
                catch(const irods::exception& _e) {
                    auto error = make_error(_e.code(), _e.what());
                    return std::forward_as_tuple(
                            Pistache::Http::Code::Bad_Request,
                            error);
                }

            } // operator()

    }; // class admin

}; // namespace irods::rest
