cmake_minimum_required(VERSION 3.5.0 FATAL_ERROR) #CPACK_DEBIAN_<COMPONENT>_PACKAGE_NAME

find_package(IRODS 4.3.0 EXACT REQUIRED CONFIG)

set(IRODS_PLUGIN_VERSION_MAJOR 1)
set(IRODS_PLUGIN_VERSION_MINOR 0)
set(IRODS_PLUGIN_VERSION_PATCH 0)
set(IRODS_PLUGIN_VERSION "${IRODS_PLUGIN_VERSION_MAJOR}.${IRODS_PLUGIN_VERSION_MINOR}.${IRODS_PLUGIN_VERSION_PATCH}")

set(CMAKE_C_COMPILER ${IRODS_EXTERNALS_FULLPATH_CLANG}/bin/clang)
set(CMAKE_CXX_COMPILER ${IRODS_EXTERNALS_FULLPATH_CLANG}/bin/clang++)
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

set(PROJECT_NAME irods_client_rest_cpp)
string(REPLACE "_" "-" PROJECT_NAME_HYPHENS ${PROJECT_NAME})

project( ${PROJECT_NAME} C CXX)
include(${IRODS_TARGETS_PATH})

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

if (NOT CPACK_PACKAGING_INSTALL_PREFIX)
  set(CPACK_PACKAGING_INSTALL_PREFIX "/" CACHE STRING "Package root path. \"/\" is correct for normal package builds.." FORCE)
  message(STATUS "Setting unspecified CPACK_PACKAGING_INSTALL_PREFIX to '${CPACK_PACKAGING_INSTALL_PREFIX}'. This is the correct setting for normal builds.")
endif()


if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build {Debug, Release}." FORCE)
  message(STATUS "Setting unspecified CMAKE_BUILD_TYPE to '${CMAKE_BUILD_TYPE}'")
endif()

if (NOT IRODS_EXTERNALS_PACKAGE_ROOT)
  set(IRODS_EXTERNALS_PACKAGE_ROOT "/opt/irods-externals" CACHE STRING "Choose the location of iRODS external packages." FORCE)
  message(STATUS "Setting unspecified IRODS_EXTERNALS_PACKAGE_ROOT to '${IRODS_EXTERNALS_PACKAGE_ROOT}'")
endif()

macro(IRODS_MACRO_SET_AND_CHECK_DEPENDENCY_FULL_PATH DEPENDENCY_NAME DEPENDENCY_SUBDIRECTORY)
  if (NOT IS_DIRECTORY ${IRODS_EXTERNALS_PACKAGE_ROOT}/${DEPENDENCY_SUBDIRECTORY})
    message(FATAL_ERROR "${DEPENDENCY_NAME} not found at ${IRODS_EXTERNALS_PACKAGE_ROOT}/${DEPENDENCY_SUBDIRECTORY}")
  endif()
  set(IRODS_EXTERNALS_FULLPATH_${DEPENDENCY_NAME} ${IRODS_EXTERNALS_PACKAGE_ROOT}/${DEPENDENCY_SUBDIRECTORY})
endmacro()

find_package(OpenSSL REQUIRED)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -stdlib=libc++")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-z,defs")
add_compile_options(-nostdinc++)
add_compile_options(-std=c++17)
link_libraries(c++abi)
include_directories(
    ${IRODS_EXTERNALS_FULLPATH_CLANG}/include/c++/v1
    )

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_COMPONENT_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_COMPONENTS_GROUPING IGNORE)
set(CPACK_PACKAGE_VERSION ${IRODS_PLUGIN_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${IRODS_PLUGIN_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${IRODS_PLUGIN_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${IRODS_PLUGIN_VERSION_PATCH})
set(CPACK_PACKAGE_CONTACT "Renaissance Computing Institute <info@irods.org>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The integrated Rule-Oriented Data System")
set(CPACK_PACKAGE_VENDOR "Renaissance Computing Institute <info@irods.org>")

set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_PACKAGE_SECTION "contrib/science")
set(CPACK_DEBIAN_COMPRESSION_TYPE "gzip")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "extra")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://irods.org")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)

set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_RPM_PACKAGE_RELEASE "1")
set(CPACK_RPM_PACKAGE_LICENSE "BSD-3-Clause")
set(CPACK_RPM_PACKAGE_VENDOR "iRODS Consortium")
set(CPACK_RPM_PACKAGE_URL "https://irods.org")
set(CPACK_RPM_PACKAGE_AUTOREQ 0)
set(CPACK_RPM_PACKAGE_AUTOPROV 0)

set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)


set(IRODS_PACKAGE_COMPONENT_PROJECT__NAME "${PROJECT__NAME_HYPHENS}${IRODS_PACKAGE_FILE_NAME_SUFFIX}")
string(TOUPPER ${IRODS_PACKAGE_COMPONENT_PROJECT__NAME} IRODS_PACKAGE_COMPONENT_PROJECT__NAME_UPPERCASE)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -std=c++17)

set(
    API_SERVER_NAMES
    Access
    Admin
    Auth
    List
    Query
    StreamGet
    StreamPut
    ZoneReport
    )

foreach(API_NAME ${API_SERVER_NAMES})
    string(TOLOWER ${API_NAME} LOWER_API_NAME)

    link_directories(/usr/local/lib/)

    aux_source_directory(model MODEL_SOURCES)

    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    set(CMAKE_INSTALL_RPATH ${IRODS_EXTERNALS_FULLPATH_CLANG_RUNTIME}/lib)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -stdlib=libc++")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
    add_compile_options(-nostdinc++ -Wall -Wextra -Werror -Wno-unused-parameter)

    include_directories(irods_specific_implementation)
    include_directories(jwt-cpp)
    include_directories(model)
    include_directories(api)
    include_directories(impl)

    set(
        EXECUTABLE_NAME
        irods_client_rest_cpp_${LOWER_API_NAME}_server
    )

    file(GLOB API_SOURCES
        "api/${API_NAME}Api.h"
        "api/${API_NAME}Api.cpp"
        "impl/${API_NAME}ApiImpl.h"
        "impl/${API_NAME}ApiImpl.cpp"
    )

    set(
        API_SERVER_SOURCES
        ${API_NAME}ApiMainServer.cpp
        ${MODEL_SOURCES}
        ${API_SOURCES}
    )

    add_executable(
        ${EXECUTABLE_NAME}
        ${API_SERVER_SOURCES}
    )

    set(IRODS_EXTERNALS_FULLPATH_PISTACHE /opt/irods-externals/pistache0.0.2-0)

    link_libraries(c++abi)
    include_directories(${IRODS_INCLUDE_DIRS}
                        ${IRODS_EXTERNALS_FULLPATH_CLANG}/include/c++/v1
                        ${IRODS_EXTERNALS_FULLPATH_JSON}/include
                        ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
                        ${IRODS_EXTERNALS_FULLPATH_FMT}/include
                        ${IRODS_EXTERNALS_FULLPATH_PISTACHE}/include)

    include_directories(model)
    include_directories(api)
    include_directories(impl)

    target_link_libraries(
        ${EXECUTABLE_NAME}
        ${IRODS_EXTERNALS_FULLPATH_PISTACHE}/lib/libpistache.a
        pthread
        crypto
        irods_client
        irods_plugin_dependencies
        irods_common
        ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so
    )

    install(
        TARGETS
        ${EXECUTABLE_NAME}
        RUNTIME
        DESTINATION usr/sbin
        COMPONENT ${IRODS_PACKAGE_COMPONENT_PROJECT_NAME}
        )
endforeach()

install(
  FILES
  ${CMAKE_SOURCE_DIR}/packaging/irods_client_rest_cpp
  DESTINATION etc/init.d
  COMPONENT ${IRODS_PACKAGE_COMPONENT_PROJECT_NAME}
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  )

install(
  FILES
  ${CMAKE_SOURCE_DIR}/packaging/irods_client_rest_cpp.json.template
  ${CMAKE_SOURCE_DIR}/packaging/irods-client-rest-cpp-reverse-proxy.conf.template
  DESTINATION etc/irods
  COMPONENT ${IRODS_PACKAGE_COMPONENT_PROJECT_NAME}
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  )

install(
  FILES
  ${CMAKE_SOURCE_DIR}/packaging/irods_rest.py
  ${CMAKE_SOURCE_DIR}/packaging/test_irods_client_rest_cpp.py
  DESTINATION var/lib/irods/scripts/irods/test
  COMPONENT ${IRODS_PACKAGE_COMPONENT_PROJECT_NAME}
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  )

set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_PROJECT_NAME_UPPERCASE}_PACKAGE_NAME ${TARGET_NAME})
set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_PROJECT_NAME_UPPERCASE}_PACKAGE_DEPENDS "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-runtime (= ${IRODS_VERSION}), libc6" )

set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_PROJECT_NAME}_PACKAGE_NAME ${TARGET_NAME})
if (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "centos" OR IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "centos linux")
    set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_SERVER__NAME}_PACKAGE_REQUIRES "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-server = ${IRODS_VERSION}, irods-runtime = ${IRODS_VERSION}, openssl")
elseif (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "opensuse")
    set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_PROJECT_NAME}_PACKAGE_REQUIRES "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-server = ${IRODS_VERSION}, irods-runtime = ${IRODS_VERSION}, libopenssl1_0_0")
endif()

set(CPACK_PACKAGE_CONTACT "Renaissance Computing Institute <info@irods.org>")
if (NOT CPACK_GENERATOR)
    set(CPACK_GENERATOR ${IRODS_CPACK_GENERATOR} CACHE STRING "CPack generator to use, e.g. {DEB, RPM, TGZ}." FORCE)
    message(STATUS "Setting unspecified CPACK_GENERATOR to ${CPACK_GENERATOR}. This is the correct setting for normal builds.")
endif()

include(CPack)

