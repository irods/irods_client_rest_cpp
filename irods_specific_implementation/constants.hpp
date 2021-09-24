#ifndef IRODS_REST_CPP_CONSTANTS_HPP
#define IRODS_REST_CPP_CONSTANTS_HPP

#include <string>

#define TO_STRING(X) #X
#define MAKE_URL(VERSION) "/irods-rest/" TO_STRING(VERSION)

namespace irods::rest
{
    // IRODS_CLIENT_VERSION is a macro that is defined by the CMakeLists.txt file.
    const std::string base_url = MAKE_URL(IRODS_CLIENT_VERSION);
} // namespace irods::rest

#endif // IRODS_REST_CPP_CONSTANTS_HPP

