#pragma once

#include <string>
#include <map>
#include <functional>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>

namespace deadlock::bridge {

struct HttpRequest {
    std::string method;
    std::string path;
    std::string query;
    std::map<std::string, std::string> headers;
    std::string body;

    std::string getHeader(const std::string& key, const std::string& def = "") const {
        for (const auto& kv : headers) {
            // Case-insensitive header lookup
            if (kv.first.size() == key.size()) {
                bool match = true;
                for (size_t i = 0; i < key.size(); ++i) {
                    if (tolower(kv.first[i]) != tolower(key[i])) {
                        match = false;
                        break;
                    }
                }
                if (match) return kv.second;
            }
        }
        return def;
    }
};

struct HttpResponse {
    int statusCode{200};
    std::string statusText{"OK"};
    std::map<std::string, std::string> headers;
    std::string body;

    static HttpResponse Ok(const std::string& body, const std::string& contentType = "application/json") {
        HttpResponse res;
        res.statusCode = 200;
        res.statusText = "OK";
        res.headers["Content-Type"] = contentType;
        res.body = body;
        return res;
    }

    static std::string escapeJsonString(const std::string& input) {
        std::string out;
        for (char c : input) {
            if (c == '"') out += "\\\"";
            else if (c == '\\') out += "\\\\";
            else if (c == '\n') out += "\\n";
            else if (c == '\r') out += "\\r";
            else if (c == '\t') out += "\\t";
            else out += c;
        }
        return out;
    }

    static HttpResponse BadRequest(const std::string& errorMsg) {
        HttpResponse res;
        res.statusCode = 400;
        res.statusText = "Bad Request";
        res.headers["Content-Type"] = "application/json";
        res.body = "{\"success\":false,\"error\":\"" + escapeJsonString(errorMsg) + "\"}";
        return res;
    }

    static HttpResponse NotFound(const std::string& errorMsg = "Resource not found") {
        HttpResponse res;
        res.statusCode = 404;
        res.statusText = "Not Found";
        res.headers["Content-Type"] = "application/json";
        res.body = "{\"success\":false,\"error\":\"" + escapeJsonString(errorMsg) + "\"}";
        return res;
    }

    static HttpResponse Unprocessable(const std::string& errorMsg) {
        HttpResponse res;
        res.statusCode = 422;
        res.statusText = "Unprocessable Entity";
        res.headers["Content-Type"] = "application/json";
        res.body = "{\"success\":false,\"error\":\"" + escapeJsonString(errorMsg) + "\"}";
        return res;
    }

    static HttpResponse InternalError(const std::string& errorMsg) {
        HttpResponse res;
        res.statusCode = 500;
        res.statusText = "Internal Server Error";
        res.headers["Content-Type"] = "application/json";
        res.body = "{\"success\":false,\"error\":\"" + escapeJsonString(errorMsg) + "\"}";
        return res;
    }

    static HttpResponse NoContent() {
        HttpResponse res;
        res.statusCode = 204;
        res.statusText = "No Content";
        return res;
    }
};

using HttpHandler = std::function<HttpResponse(const HttpRequest&)>;

/**
 * @brief High-performance, lightweight HTTP/1.1 server implementation using Winsock2.
 */
class HttpServer {
public:
    explicit HttpServer(int port = 8080, const std::string& staticDir = "frontend");
    ~HttpServer();

    void setRoute(const std::string& method, const std::string& path, HttpHandler handler);
    void start(bool async = true);
    void stop();
    bool isRunning() const { return running_; }
    int getPort() const { return port_; }

private:
    int port_;
    std::string staticDir_;
    std::atomic<bool> running_{false};
    std::unique_ptr<std::thread> listenerThread_;
    std::map<std::pair<std::string, std::string>, HttpHandler> routes_;

    void runServer();
    void handleClient(uintptr_t clientSocket);
    HttpResponse processRequest(const HttpRequest& req);
    bool serveStaticFile(const std::string& reqPath, HttpResponse& outRes);
};

} // namespace deadlock::bridge
