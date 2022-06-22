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
* ZoneReportApiImpl.h
*
*
*/

#ifndef ZONE_REPORT_API_IMPL_H_
#define ZONE_REPORT_API_IMPL_H_

#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/router.h>
#include <pistache/optional.h>

#include "ModelBase.h"

#include <ZoneReportApi.h>

#include <memory>
#include <string>

#include "irods_rest_zone_report_api_implementation.h"

namespace io::swagger::server::api
{
    using namespace io::swagger::server::model;

    class ZoneReportApiImpl
        : public io::swagger::server::api::ZoneReportApi
    {
    public:
        ZoneReportApiImpl(Pistache::Address addr);

        ~ZoneReportApiImpl() = default;

        void handler_impl(const Pistache::Rest::Request& request,
                          Pistache::Http::ResponseWriter& response) override;

        irods::rest::zone_report irods_zone_report_;
    }; // class ZoneReportApiImpl
} // namespace io::swagger::server::api

#endif // ZONE_REPORT_API_IMPL_H_

