#pragma once

#include "bridge/HttpServer.hpp"
#include "bridge/SimulationService.hpp"

#include <memory>

namespace deadlock::bridge {

/**
 * @brief Binds RESTful JSON API endpoints to the SimulationService business logic.
 */
class ApiRouter {
public:
    static void registerRoutes(HttpServer& server, std::shared_ptr<SimulationService> service);
};

} // namespace deadlock::bridge
