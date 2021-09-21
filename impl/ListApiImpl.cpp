/**
* iRODS REST API
* This is the iRODS REST API
*
* OpenAPI spec version: 1.0.0
* Contact: info@irods.org
*
* NOTE: This class is auto generated by the swagger code generator program.
* https://github.com/swagger-api/swagger-codegen.git
* Do not edit the class manually.
*/

#include "ListApiImpl.h"

namespace io::swagger::server::api
{
    using namespace io::swagger::server::model;

    ListApiImpl::ListApiImpl(Pistache::Address addr)
        : ListApi(addr)
        , irods_list_{}
    {
    }

    void ListApiImpl::handler_impl(const Pistache::Rest::Request& request,
                                   Pistache::Http::ResponseWriter& response)
    {
        auto [code, msg] = irods_list_(request, response);
        response.send(code, msg);
    }
} // namespace io::swagger::server::api

