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

/*
* AccessApiImpl.h
*
* 
*/

#ifndef ACCESS_API_IMPL_H_
#define ACCESS_API_IMPL_H_

#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/router.h>
#include <pistache/optional.h>

#include "ModelBase.h"

#include <AccessApi.h>

#include <memory>
#include <string>

#include "irods_rest_access_implementation.h"

namespace io::swagger::server::api
{
    using namespace io::swagger::server::model;

    class AccessApiImpl
        : public io::swagger::server::api::AccessApi
    {
    public:
        AccessApiImpl(Pistache::Address addr);

        ~AccessApiImpl() = default;

        void handler_impl(const Pistache::Http::Header::Collection& headers,
                          const std::string& body,
                          const Pistache::Optional<std::string>& path,
                          const Pistache::Optional<std::string>& use_count,
                          const Pistache::Optional<std::string>& seconds_until_expiration,
                          Pistache::Http::ResponseWriter& response) override;

        irods::rest::access irods_access_;
    }; // class AccessApiImpl
} // namespace io::swagger::server::api

#endif // ACCESS_API_IMPL_H_

