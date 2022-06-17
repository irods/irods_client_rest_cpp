/*
* MetadataApiImpl.h
*
*
*/

#ifndef METADATA_API_IMPL_H_
#define METADATA_API_IMPL_H_

#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/router.h>
#include <pistache/optional.h>

#include "ModelBase.h"

#include <MetadataApi.h>

#include <memory>
#include <string>

#include "irods_rest_metadata_api_implementation.h"

namespace io::swagger::server::api
{
    using namespace io::swagger::server::model;

    class MetadataApiImpl
        : public io::swagger::server::api::MetadataApi
    {
    public:
        MetadataApiImpl(Pistache::Address addr);

        ~MetadataApiImpl() = default;

        void handler_impl(const Pistache::Rest::Request& request,
                          Pistache::Http::ResponseWriter& response) override;

        irods::rest::metadata irods_metadata_;
    }; // class MetaApiImpl
} // namespace io::swagger::server::api

#endif // METADATA_API_IMPL_H_
