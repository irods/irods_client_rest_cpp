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
* StreamGetApiImpl.h
*
* 
*/

#ifndef STREAM_GET_API_IMPL_H_
#define STREAM_GET_API_IMPL_H_

#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/router.h>
#include <pistache/optional.h>

#include "ModelBase.h"

#include <StreamGetApi.h>

#include <memory>
#include <string>

#include "irods_rest_stream_get_api_implementation.h"

namespace io::swagger::server::api
{
    using namespace io::swagger::server::model;

    class StreamGetApiImpl
        : public io::swagger::server::api::StreamGetApi
    {
    public:
        StreamGetApiImpl(Pistache::Address addr);

        ~StreamGetApiImpl() = default;

        void handler_impl(const Pistache::Rest::Request& request,
                          Pistache::Http::ResponseWriter& response) override;

        irods::rest::stream irods_stream_get_;
    }; // class StreamGetApiImpl
} // namespace io::swagger::server::api

#endif // STREAM_GET_API_IMPL_H_

