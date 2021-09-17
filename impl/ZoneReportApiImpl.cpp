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

#include "ZoneReportApiImpl.h"

namespace io::swagger::server::api
{
    using namespace io::swagger::server::model;

    ZoneReportApiImpl::ZoneReportApiImpl(Pistache::Address addr)
        : ZoneReportApi(addr)
        , irods_zone_report_{}
    {
    }

    void ZoneReportApiImpl::obtain_token(const Pistache::Http::Header::Collection& headers,
                                         const std::string& body,
                                         Pistache::Http::ResponseWriter& response)
    {
        MACRO_IRODS_ZONE_REPORT_API_IMPLEMENTATION
    }
} // namespace io::swagger::server::api

