cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)

find_package(IRODS 4.3.0 EXACT REQUIRED CONFIG)

set(IRODS_CLIENT_VERSION_MAJOR "0")
set(IRODS_CLIENT_VERSION_MINOR "9")
set(IRODS_CLIENT_VERSION_PATCH "4")
set(IRODS_CLIENT_VERSION "${IRODS_CLIENT_VERSION_MAJOR}.${IRODS_CLIENT_VERSION_MINOR}.${IRODS_CLIENT_VERSION_PATCH}")

set(IRODS_PACKAGE_REVISION "0")

include(IrodsCXXCompiler)
set(CMAKE_CXX_STANDARD ${IRODS_CXX_STANDARD})
set(CMAKE_EXE_LINKER_FLAGS_INIT "-rdynamic -Wl,--export-dynamic -Wl,--enable-new-dtags -Wl,--as-needed -Wl,-z,defs")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE_INIT "-Wl,--gc-sections -Wl,-z,combreloc")

set(PROJECT_NAME irods_client_rest_cpp)
string(REPLACE "_" "-" PROJECT_NAME_HYPHENS ${PROJECT_NAME})

set(IRODS_CLIENT_REST_CPP_DIRNAME "${PROJECT_NAME}")

include(IrodsExternals)

IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH(PISTACHE pistache0.0.2-0)

project(${PROJECT_NAME}
        VERSION "${IRODS_CLIENT_VERSION}"
        LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_SKIP_BUILD_RPATH OFF)
set(CMAKE_SKIP_INSTALL_RPATH OFF)
set(CMAKE_SKIP_RPATH OFF)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)
set(CMAKE_BUILD_WITH_INSTALL_RPATH OFF)
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)

include(${IRODS_TARGETS_PATH})

include(GNUInstallDirs)

if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build {Debug, Release}." FORCE)
  message(STATUS "Setting unspecified CMAKE_BUILD_TYPE to '${CMAKE_BUILD_TYPE}'")
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
  set(IRODS_BUILD_WITH_WERROR_DEFAULT ON)
else()
  set(IRODS_BUILD_WITH_WERROR_DEFAULT OFF)
endif()

set(IRODS_BUILD_WITH_WERROR ${IRODS_BUILD_WITH_WERROR_DEFAULT} CACHE BOOL "Choose whether to compile with -Werror.")

include(UseLibCXX)

if (NOT DEFINED THREADS_PREFER_PTHREAD_FLAG)
  set(THREADS_PREFER_PTHREAD_FLAG TRUE)
endif()
find_package(Threads REQUIRED)
find_package(nlohmann_json "3.6.1" REQUIRED)
find_package(OpenSSL REQUIRED)

add_definitions(-DIRODS_CLIENT_VERSION=${IRODS_CLIENT_VERSION})

if (IRODS_BUILD_WITH_WERROR)
  add_compile_options(-Werror)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fpermissive>)
endif()

add_compile_options(-Wall -Wextra -Wno-unused-parameter)

set(
  irods_client_rest_cpp_common
  "irods_client_rest_cpp_common"
)

add_library(
  irods_client_rest_cpp_common
  OBJECT
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_specific_implementation/configuration.cpp"
)

target_include_directories(
  irods_client_rest_cpp_common
  PRIVATE
  ${IRODS_INCLUDE_DIRS}
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_FMT}/include
)

target_link_libraries(
  irods_client_rest_cpp_common
  PRIVATE
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
  ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so
)

target_link_libraries(
  irods_client_rest_cpp_common
  PUBLIC
  nlohmann_json::nlohmann_json
)

set(MODEL_SOURCES
    ${CMAKE_SOURCE_DIR}/model/ModelBase.cpp
    ${CMAKE_SOURCE_DIR}/model/Query_results.cpp
    )

set(API_SERVER_NAMES
    Admin
    Auth
    GetConfiguration
    List
    LogicalPath
    Metadata
    PutConfiguration
    Query
    StreamGet
    StreamPut
    Ticket
    ZoneReport
    )

foreach(API_NAME ${API_SERVER_NAMES})
    string(TOLOWER ${API_NAME} LOWER_API_NAME)
    set(EXECUTABLE_NAME irods_client_rest_cpp_${LOWER_API_NAME}_server)

    add_executable(
        ${EXECUTABLE_NAME}
        ${CMAKE_SOURCE_DIR}/${API_NAME}ApiMainServer.cpp
        ${CMAKE_SOURCE_DIR}/api/${API_NAME}Api.cpp
        ${CMAKE_SOURCE_DIR}/impl/${API_NAME}ApiImpl.cpp
        ${MODEL_SOURCES}
        )

    target_include_directories(
        ${EXECUTABLE_NAME}
        PRIVATE
        irods_specific_implementation
        jwt-cpp
        model
        api
        impl
        )

    target_include_directories(
        ${EXECUTABLE_NAME}
        PRIVATE
        ${IRODS_INCLUDE_DIRS}
        ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
        ${IRODS_EXTERNALS_FULLPATH_FMT}/include
        ${IRODS_EXTERNALS_FULLPATH_SPDLOG}/include
        ${IRODS_EXTERNALS_FULLPATH_PISTACHE}/include
        )

    target_link_libraries(
        ${EXECUTABLE_NAME}
        PRIVATE
        irods_client
        irods_plugin_dependencies
        irods_common
        irods_client_rest_cpp_common
        ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so
        ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
        ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
        ${IRODS_EXTERNALS_FULLPATH_PISTACHE}/lib/libpistache.a
        OpenSSL::Crypto
        Threads::Threads
        nlohmann_json::nlohmann_json
        )

    target_compile_definitions(${EXECUTABLE_NAME} PRIVATE ${IRODS_COMPILE_DEFINITIONS_PRIVATE})

    install(
        TARGETS
        ${EXECUTABLE_NAME}
        RUNTIME
        DESTINATION ${CMAKE_INSTALL_SBINDIR}
        COMPONENT ${IRODS_PACKAGE_COMPONENT_PROJECT_NAME}
        )
endforeach()

# Generate the nginx reverse proxy template.
configure_file(${CMAKE_SOURCE_DIR}/packaging/irods_client_rest_cpp_reverse_proxy.conf.template.in
               ${CMAKE_BINARY_DIR}/packaging/irods_client_rest_cpp_reverse_proxy.conf.template
               @ONLY)

install(
  FILES
  ${CMAKE_SOURCE_DIR}/packaging/irods_client_rest_cpp
  DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/init.d
  COMPONENT ${IRODS_PACKAGE_COMPONENT_PROJECT_NAME}
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  )

install(
  FILES
  ${CMAKE_SOURCE_DIR}/packaging/irods_client_rest_cpp.conf.rsyslog
  ${CMAKE_SOURCE_DIR}/packaging/irods_client_rest_cpp.logrotate
  ${CMAKE_SOURCE_DIR}/packaging/irods_client_rest_cpp.json.template
  ${CMAKE_BINARY_DIR}/packaging/irods_client_rest_cpp_reverse_proxy.conf.template
  DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/${IRODS_CLIENT_REST_CPP_DIRNAME}
  COMPONENT ${IRODS_PACKAGE_COMPONENT_PROJECT_NAME}
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  )

install(
  FILES
  ${CMAKE_SOURCE_DIR}/packaging/irods_rest.py
  ${CMAKE_SOURCE_DIR}/packaging/test_irods_client_rest_cpp.py
  DESTINATION ${IRODS_HOME_DIRECTORY}/scripts/irods/test
  COMPONENT ${IRODS_PACKAGE_COMPONENT_PROJECT_NAME}
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  )

if (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "ubuntu" OR IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "debian")
  if (NOT IRODS_PLUGIN_LINUX_DISTRIBUTION_VERSION_CODENAME)
    execute_process(
      COMMAND "lsb_release" "-s" "-c"
      RESULT_VARIABLE IRODS_PLUGIN_EXECUTE_PROCESS_RESULT_LINUX_DISTRIBUTION_VERSION_CODENAME
      OUTPUT_VARIABLE IRODS_PLUGIN_LINUX_DISTRIBUTION_VERSION_CODENAME
      )
    string(STRIP ${IRODS_PLUGIN_LINUX_DISTRIBUTION_VERSION_CODENAME} IRODS_PLUGIN_LINUX_DISTRIBUTION_VERSION_CODENAME)
    if (NOT ${IRODS_PLUGIN_EXECUTE_PROCESS_RESULT_LINUX_DISTRIBUTION_VERSION_CODENAME} STREQUAL "0")
      message(FATAL_ERROR "Linux lsb_release shortname detection failed\n${IRODS_PLUGIN_EXECUTE_PROCESS_RESULT_LINUX_DISTRIBUTION_VERSION_CODENAME}")
    endif()
    set(IRODS_PLUGIN_LINUX_DISTRIBUTION_VERSION_CODENAME ${IRODS_PLUGIN_LINUX_DISTRIBUTION_VERSION_CODENAME} CACHE STRING "Linux distribution version codename, e.g. {precise, wheezy, trusty, jessie, ...}." FORCE)
    message(STATUS "Setting unspecified IRODS_PLUGIN_LINUX_DISTRIBUTION_VERSION_CODENAME to '${IRODS_PLUGIN_LINUX_DISTRIBUTION_VERSION_CODENAME}'")
  endif()
endif()

if (NOT CPACK_GENERATOR)
    set(CPACK_GENERATOR ${IRODS_CPACK_GENERATOR} CACHE STRING "CPack generator to use, e.g. {DEB, RPM, TGZ}." FORCE)
    message(STATUS "Setting unspecified CPACK_GENERATOR to ${CPACK_GENERATOR}. This is the correct setting for normal builds.")
endif()

include(IrodsCPackCommon)

set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "/usr/sbin")
list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "${CPACK_PACKAGING_INSTALL_PREFIX}${CMAKE_INSTALL_SYSCONFDIR}")
list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "${CPACK_PACKAGING_INSTALL_PREFIX}${CMAKE_INSTALL_SYSCONFDIR}/${IRODS_CLIENT_REST_CPP_DIRNAME}")
list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "${CPACK_PACKAGING_INSTALL_PREFIX}${IRODS_HOME_DIRECTORY}")
list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "${CPACK_PACKAGING_INSTALL_PREFIX}${IRODS_HOME_DIRECTORY}/scripts")
list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "${CPACK_PACKAGING_INSTALL_PREFIX}${IRODS_HOME_DIRECTORY}/scripts/irods")
list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "${CPACK_PACKAGING_INSTALL_PREFIX}${IRODS_HOME_DIRECTORY}/scripts/irods/test")

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_COMPONENT_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_COMPONENTS_GROUPING IGNORE)
set(CPACK_PACKAGE_VERSION ${IRODS_CLIENT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The integrated Rule-Oriented Data System")

set(CPACK_DEB_COMPONENT_INSTALL OFF)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)
set(CPACK_DEBIAN_PACKAGE_RELEASE "${IRODS_PACKAGE_REVISION}")
if (IRODS_LINUX_DISTRIBUTION_VERSION_CODENAME)
  set(CPACK_DEBIAN_PACKAGE_RELEASE "${CPACK_DEBIAN_PACKAGE_RELEASE}~${IRODS_LINUX_DISTRIBUTION_VERSION_CODENAME}")
endif()
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

set(CPACK_RPM_COMPONENT_INSTALL OFF)
set(CPACK_RPM_PACKAGE_LICENSE "BSD-3-Clause")
set(CPACK_RPM_PACKAGE_AUTOREQ 0)
set(CPACK_RPM_PACKAGE_AUTOPROV 0)
set(CPACK_RPM_PACKAGE_RELEASE "${IRODS_PACKAGE_REVISION}")
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)

set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)

set(CPACK_DEBIAN_PACKAGE_NAME "${PROJECT_NAME_HYPHENS}")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-runtime (= ${IRODS_VERSION}), libc6" )

set(CPACK_RPM_PACKAGE_NAME "${PROJECT_NAME_HYPHENS}")
if (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "centos" OR IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "centos linux")
  set(CPACK_RPM_PACKAGE_REQUIRES "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-runtime = ${IRODS_VERSION}")
elseif (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "opensuse")
  set(CPACK_RPM_PACKAGE_REQUIRES "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-runtime = ${IRODS_VERSION}, libopenssl1_0_0")
endif()

include(CPack)
