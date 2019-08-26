/**
* iRODS Query API
* This is the iRODS Query API
*
* OpenAPI spec version: 1.0.0
* Contact: info@irods.org
*
* NOTE: This class is auto generated by the swagger code generator program.
* https://github.com/swagger-api/swagger-codegen.git
* Do not edit the class manually.
*/
/*
 * QueryApi.h
 *
 * 
 */

#ifndef QueryApi_H_
#define QueryApi_H_


#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/router.h>
#include <pistache/http_headers.h>
#include <pistache/optional.h>

#include "Query_results.h"
#include <string>

namespace io {
namespace swagger {
namespace server {
namespace api {

using namespace io::swagger::server::model;

class  QueryApi {
public:
    QueryApi(Pistache::Address addr);
    virtual ~QueryApi() {};
    void init(size_t thr);
    void start();
    void shutdown();

    const std::string base = "/jasoncoposky/irods-rest/1.0.0";

private:
    void setupRoutes();

    void catalog_query_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);
    void query_api_default_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);

    std::shared_ptr<Pistache::Http::Endpoint> httpEndpoint;
    Pistache::Rest::Router router;


    /// <summary>
    /// searches iRODS Catalog using the General Query Language
    /// </summary>
    /// <remarks>
    /// By passing in the appropriate options, you can search for anything within the iRODS Catalog 
    /// </remarks>
    /// <param name="queryString">pass an query string using the general query language</param>
    /// <param name="rowOffset">number of records to skip for pagination (optional)</param>
    /// <param name="queryLimit">maximum number of records to return (optional)</param>
    virtual void catalog_query(const Pistache::Optional<std::string> &queryString, const Pistache::Optional<std::string> &rowOffset, const Pistache::Optional<std::string> &queryLimit, Pistache::Http::ResponseWriter &response) = 0;

};

}
}
}
}

#endif /* QueryApi_H_ */

