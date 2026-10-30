#include "bridge/HttpServer.hpp"
#include "bridge/SimulationService.hpp"
#include "bridge/ApiRouter.hpp"

#include <iostream>
#include <string>
#include <csignal>
#include <memory>

using namespace deadlock::bridge;

static std::unique_ptr<HttpServer> g_server;

void handleSignal(int) {
    std::cout << "\n[Server] Shutting down Deadlock Engine HTTP Bridge..." << std::endl;
    if (g_server) {
        g_server->stop();
    }
}

int main(int argc, char* argv[]) {
    int port = 8080;
    std::string webDir = "frontend";

    if (argc > 1) {
        try {
            port = std::stoi(argv[1]);
        } catch (...) {
            std::cerr << "Invalid port: " << argv[1] << ", using default 8080." << std::endl;
        }
    }
    if (argc > 2) {
        webDir = argv[2];
    }

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    std::cout << "========================================================\n";
    std::cout << "  DEADLOCK SIMULATION ENGINE - HTTP API & WEB BRIDGE\n";
    std::cout << "  Course-Based Project (CBP) - Phase 7 Integration\n";
    std::cout << "========================================================\n";
    std::cout << "  Web UI:        http://localhost:" << port << "/\n";
    std::cout << "  API Base URL:  http://localhost:" << port << "/api/state\n";
    std::cout << "  Static Dir:    " << webDir << "\n";
    std::cout << "========================================================\n\n";

    auto service = std::make_shared<SimulationService>();
    g_server = std::make_unique<HttpServer>(port, webDir);

    ApiRouter::registerRoutes(*g_server, service);

    std::cout << "[Server] Ready. Press Ctrl+C to terminate.\n" << std::endl;
    g_server->start(false); // Run synchronously on main thread

    return 0;
}
