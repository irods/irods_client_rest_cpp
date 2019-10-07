cmake_minimum_required(VERSION 3.5.0 FATAL_ERROR) #CPACK_DEBIAN_<COMPONENT>_PACKAGE_NAME

find_package(IRODS 4.3.0 EXACT REQUIRED CONFIG)


set(CMAKE_C_COMPILER ${IRODS_EXTERNALS_FULLPATH_CLANG}/bin/clang)
set(CMAKE_CXX_COMPILER ${IRODS_EXTERNALS_FULLPATH_CLANG}/bin/clang++)
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

project(server CXX)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -std=c++17)
set(
    API_SERVER_NAMES
    Query
    Auth
    Stream
    )

foreach(API_NAME ${API_SERVER_NAMES})
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
        ${API_NAME}ApiMainServer
    )

    file(GLOB API_SOURCES
        "api/${API_NAME}Api.h"
        "api/${API_NAME}Api.cpp"
        "impl/${API_NAME}ApiImpl.h"
        "impl/${API_NAME}ApiImpl.cpp"
    )

    set(
        API_SERVER_SOURCES
        ${EXECUTABLE_NAME}.cpp
        ${MODEL_SOURCES}
        ${API_SOURCES}
    )

    add_executable(
        ${EXECUTABLE_NAME}
        ${API_SERVER_SOURCES}
    )

    link_libraries(c++abi)
    include_directories(${IRODS_EXTERNALS_FULLPATH_CLANG}/include/c++/v1
                        ${IRODS_EXTERNALS_FULLPATH_JSON}/include
                        ${IRODS_EXTERNALS_FULLPATH_SPDLOG}/include)

    include_directories(model)
    include_directories(api)
    include_directories(impl)

    include_directories(${IRODS_INCLUDE_DIRS})
    include_directories(${IRODS_EXTERNALS_FULLPATH_AVRO}/include)
    include_directories(${IRODS_EXTERNALS_FULLPATH_BOOST}/include)
    include_directories(${IRODS_EXTERNALS_FULLPATH_ARCHIVE}/include)
    include_directories(${IRODS_EXTERNALS_FULLPATH_CPPZMQ}/include)
    include_directories(${IRODS_EXTERNALS_FULLPATH_ZMQ}/include)
    #include_directories(${IRODS_EXTERNALS_FULLPATH_JSON}/include)
    include_directories(/opt/irods-externals/json3.4.0-0/include)

    target_link_libraries(
        ${EXECUTABLE_NAME}
        pistache
        pthread
        crypto
        irods_client
        irods_plugin_dependencies
        irods_common
        ${IRODS_EXTERNALS_FULLPATH_AVRO}/lib/libavrocpp.so
        ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
        ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_program_options.so
        ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
        ${IRODS_EXTERNALS_FULLPATH_ZMQ}/lib/libzmq.so
    )

endforeach()
