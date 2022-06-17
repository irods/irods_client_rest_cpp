#include "MetadataApiImpl.h"

#include "utils.hpp"

namespace io::swagger::server::api
{
    MetadataApiImpl::MetadataApiImpl(Pistache::Address addr)
        : MetadataApi(addr)
        , irods_metadata_{}
    {
    }

    void MetadataApiImpl::handler_impl(const Pistache::Rest::Request& request,
                                    Pistache::Http::ResponseWriter& response)
    {
        irods::rest::handle_request(irods_metadata_, request, response);
    }
} // namespace io::swagger::server::api

