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

#include "PutConfigurationApiImpl.h"

#include "utils.hpp"

namespace io::swagger::server::api
{
    using namespace io::swagger::server::model;

    PutConfigurationApiImpl::PutConfigurationApiImpl(Pistache::Address addr)
        : PutConfigurationApi(addr)
        , irods_put_configuration_{}
    {
    }

    void PutConfigurationApiImpl::handler_impl(const Pistache::Rest::Request& request,
                                               Pistache::Http::ResponseWriter& response)
    {
        irods::rest::handle_request(irods_put_configuration_, request, response);
    }
} // namespace io::swagger::server::api

