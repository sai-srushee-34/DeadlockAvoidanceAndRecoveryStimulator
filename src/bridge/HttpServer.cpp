#include "bridge/HttpServer.hpp"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>

namespace deadlock::bridge {

namespace {
std::string getMimeType(const std::string& path) {
    if (path.rfind(".html") != std::string::npos || path.rfind(".htm") != std::string::npos) return "text/html; charset=utf-8";
    if (path.rfind(".css") != std::string::npos) return "text/css; charset=utf-8";
    if (path.rfind(".js") != std::string::npos) return "application/javascript; charset=utf-8";
    if (path.rfind(".json") != std::string::npos) return "application/json; charset=utf-8";
    if (path.rfind(".png") != std::string::npos) return "image/png";
    if (path.rfind(".jpg") != std::string::npos || path.rfind(".jpeg") != std::string::npos) return "image/jpeg";
    if (path.rfind(".svg") != std::string::npos) return "image/svg+xml";
    if (path.rfind(".ico") != std::string::npos) return "image/x-icon";
    return "text/plain; charset=utf-8";
}
} // namespace

HttpServer::HttpServer(int port, const std::string& staticDir)
    : port_(port), staticDir_(staticDir) {
#if defined(_WIN32)
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

HttpServer::~HttpServer() {
    stop();
#if defined(_WIN32)
    WSACleanup();
#endif
}

void HttpServer::setRoute(const std::string& method, const std::string& path, HttpHandler handler) {
    routes_[{method, path}] = std::move(handler);
}

void HttpServer::start(bool async) {
    if (running_) return;
    running_ = true;

    if (async) {
        listenerThread_ = std::make_unique<std::thread>(&HttpServer::runServer, this);
    } else {
        runServer();
    }
}

void HttpServer::stop() {
    if (!running_) return;
    running_ = false;

    // Wake up accept by connecting to localhost
#if defined(_WIN32)
    SOCKET dummy = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (dummy != INVALID_SOCKET) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<u_short>(port_));
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        connect(dummy, (sockaddr*)&addr, sizeof(addr));
        closesocket(dummy);
    }
#endif

    if (listenerThread_ && listenerThread_->joinable()) {
        listenerThread_->join();
        listenerThread_.reset();
    }
}

void HttpServer::runServer() {
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "[HttpServer] Failed to create socket." << std::endl;
        running_ = false;
        return;
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(static_cast<u_short>(port_));

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "[HttpServer] Failed to bind to port " << port_ << std::endl;
        closesocket(serverSocket);
        running_ = false;
        return;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "[HttpServer] Listen failed." << std::endl;
        closesocket(serverSocket);
        running_ = false;
        return;
    }

    std::cout << "[HttpServer] Deadlock Engine HTTP API listening on http://127.0.0.1:" << port_ << std::endl;

    while (running_) {
        sockaddr_in clientAddr{};
        int clientLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientLen);

        if (!running_) {
            if (clientSocket != INVALID_SOCKET) closesocket(clientSocket);
            break;
        }

        if (clientSocket == INVALID_SOCKET) continue;

        // Process client request in a detached thread for multi-concurrency
        std::thread([this, clientSocket]() {
            this->handleClient(static_cast<uintptr_t>(clientSocket));
        }).detach();
    }

    closesocket(serverSocket);
}

void HttpServer::handleClient(uintptr_t clientSockHandle) {
    SOCKET client = static_cast<SOCKET>(clientSockHandle);

    char buffer[4096];
    std::string rawRequest;
    int bytesReceived = 0;

    // Read headers
    size_t headerEnd = std::string::npos;
    while ((bytesReceived = recv(client, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytesReceived] = '\0';
        rawRequest.append(buffer, bytesReceived);

        headerEnd = rawRequest.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            break;
        }
    }

    if (rawRequest.empty()) {
        closesocket(client);
        return;
    }

    std::istringstream reqStream(rawRequest.substr(0, headerEnd));
    std::string requestLine;
    if (!std::getline(reqStream, requestLine)) {
        closesocket(client);
        return;
    }

    if (!requestLine.empty() && requestLine.back() == '\r') requestLine.pop_back();

    std::istringstream lineStream(requestLine);
    HttpRequest req;
    lineStream >> req.method >> req.path;

    // Parse query string if present
    size_t qPos = req.path.find('?');
    if (qPos != std::string::npos) {
        req.query = req.path.substr(qPos + 1);
        req.path = req.path.substr(0, qPos);
    }

    // Parse headers
    std::string headerLine;
    while (std::getline(reqStream, headerLine)) {
        if (!headerLine.empty() && headerLine.back() == '\r') headerLine.pop_back();
        if (headerLine.empty()) continue;

        size_t colon = headerLine.find(':');
        if (colon != std::string::npos) {
            std::string k = headerLine.substr(0, colon);
            std::string v = headerLine.substr(colon + 1);
            while (!v.empty() && v.front() == ' ') v.erase(v.begin());
            req.headers[k] = v;
        }
    }

    // Read remaining body if Content-Length > 0
    size_t contentLength = 0;
    std::string clStr = req.getHeader("Content-Length");
    if (!clStr.empty()) {
        try {
            contentLength = std::stoul(clStr);
        } catch (...) {
            contentLength = 0;
        }
    }

    std::string currentBody = rawRequest.substr(headerEnd + 4);
    while (currentBody.size() < contentLength) {
        int r = recv(client, buffer, sizeof(buffer) - 1, 0);
        if (r <= 0) break;
        currentBody.append(buffer, r);
    }
    req.body = currentBody;

    // Process request & produce response
    HttpResponse res = processRequest(req);

    // Build raw HTTP response
    std::ostringstream responseStream;
    responseStream << "HTTP/1.1 " << res.statusCode << " " << res.statusText << "\r\n";
    responseStream << "Access-Control-Allow-Origin: *\r\n";
    responseStream << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n";
    responseStream << "Access-Control-Allow-Headers: Content-Type, Authorization, X-Requested-With\r\n";
    responseStream << "Connection: close\r\n";

    for (const auto& h : res.headers) {
        responseStream << h.first << ": " << h.second << "\r\n";
    }
    responseStream << "Content-Length: " << res.body.size() << "\r\n\r\n";
    responseStream << res.body;

    std::string responseStr = responseStream.str();
    send(client, responseStr.c_str(), static_cast<int>(responseStr.size()), 0);

    closesocket(client);
}

HttpResponse HttpServer::processRequest(const HttpRequest& req) {
    // Handle CORS pre-flight
    if (req.method == "OPTIONS") {
        return HttpResponse::NoContent();
    }

    // Check registered API routes
    auto routeIt = routes_.find({req.method, req.path});
    if (routeIt != routes_.end()) {
        try {
            return routeIt->second(req);
        } catch (const std::exception& ex) {
            return HttpResponse::InternalError(std::string("Handler error: ") + ex.what());
        } catch (...) {
            return HttpResponse::InternalError("Unknown exception occurred during route dispatch.");
        }
    }

    // If method is GET and not an /api route, attempt to serve static files from staticDir
    if (req.method == "GET" && req.path.rfind("/api/", 0) != 0) {
        HttpResponse staticRes;
        if (serveStaticFile(req.path, staticRes)) {
            return staticRes;
        }
    }

    return HttpResponse::NotFound("Endpoint or file not found: " + req.method + " " + req.path);
}

bool HttpServer::serveStaticFile(const std::string& reqPath, HttpResponse& outRes) {
    std::string relPath = reqPath;
    if (relPath == "/" || relPath.empty()) {
        relPath = "/index.html";
    }

    // Prevent directory traversal attacks
    if (relPath.find("..") != std::string::npos) {
        return false;
    }

    std::string fullPath = staticDir_ + relPath;
    std::ifstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
        // Fallback check between frontend and web paths
        if (staticDir_ == "frontend") {
            std::string altPath = "web" + relPath;
            file.open(altPath, std::ios::binary);
            if (file.is_open()) {
                fullPath = altPath;
            }
        } else if (staticDir_ == "web") {
            std::string altPath = "frontend" + relPath;
            file.open(altPath, std::ios::binary);
            if (file.is_open()) {
                fullPath = altPath;
            }
        }
    }
    if (!file.is_open()) {
        return false;
    }

    std::ostringstream ss;
    ss << file.rdbuf();

    outRes = HttpResponse::Ok(ss.str(), getMimeType(fullPath));
    return true;
}

} // namespace deadlock::bridge
