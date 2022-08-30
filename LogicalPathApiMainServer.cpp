#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/router.h>
#include "LogicalPathApiImpl.h"

#include "logger.hpp"

using namespace io::swagger::server::api;

// clang-format off
namespace ir   = irods::rest;
namespace irck = irods::rest::configuration_keywords;
// clang-format on

int main()
{
    try {
        ir::configuration::init();

        const auto& cfg = ir::configuration::rest_service(ir::service_name);

        auto logger = ir::init_logger(ir::service_name);

        auto port = cfg[irck::port];
        if (port.empty()) {
            logger->error("Port is not configured for service.");
            return 3;
        }

        auto threads = cfg[irck::threads];
        if (threads.empty()) {
            logger->info("Using default number of threads [4].");
            threads = 4;
        }

        auto addr = Pistache::Address(Pistache::Ipv4::any(), Pistache::Port(port.get<int>()));
        auto server = LogicalPathApiImpl(addr);
        server.init(threads.get<int>());
        server.start();

        server.shutdown();
    }
    catch (...) {
        return 1;
    }
}
