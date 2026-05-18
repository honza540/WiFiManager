#ifndef ARDUINO_HOST_WEB_SERVER_H
#define ARDUINO_HOST_WEB_SERVER_H

#include "ArduinoHostString.h"

#include <functional>
#include <map>
#include <string>

// ============================================================================
// WEB SERVER STUB
// ============================================================================

using THandlerFunction = std::function<void()>;

enum HTTPMethod {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE
};

class WebServer {
public:
    explicit WebServer(int port) : serverPort(port) {}
    void on(const char* uri, HTTPMethod method, THandlerFunction handler) {
        routes[routeKey(uri, method)] = handler;
    }
    void begin() {
        startedFlag = true;
    }
    void handleClient() {}
    void stop() {
        startedFlag = false;
    }
    void send(int code, const char* contentType, const String& content) {
        lastStatusCode = code;
        lastContentType = contentType;
        lastBody = content;
    }
    bool hasArg(const char* name) const {
        return args.find(name ? name : "") != args.end();
    }
    String arg(const char* name) const {
        auto it = args.find(name ? name : "");
        return it != args.end() ? it->second : String("");
    }

    bool isStarted() const {
        return startedFlag;
    }
    int port() const {
        return serverPort;
    }
    bool routeRegistered(const char* uri, HTTPMethod method) const {
        return routes.find(routeKey(uri, method)) != routes.end();
    }
    void setArg(const char* name, const String& value) {
        args[name ? name : ""] = value;
    }
    void clearArgs() {
        args.clear();
    }
    bool simulateRequest(const char* uri, HTTPMethod method) {
        clearLastResponse();
        auto it = routes.find(routeKey(uri, method));
        if (it == routes.end()) {
            return false;
        }
        it->second();
        return true;
    }
    int getLastStatusCode() const {
        return lastStatusCode;
    }
    String getLastContentType() const {
        return lastContentType;
    }
    String getLastBody() const {
        return lastBody;
    }
    void clearLastResponse() {
        lastStatusCode = 0;
        lastContentType = "";
        lastBody = "";
    }

private:
    static string routeKey(const char* uri, HTTPMethod method) {
        return std::to_string(static_cast<int>(method)) + ":" + (uri ? uri : "");
    }

    int serverPort;
    bool startedFlag = false;
    int lastStatusCode = 0;
    String lastContentType = "";
    String lastBody = "";
    std::map<string, THandlerFunction> routes;
    std::map<string, String> args;
};

#endif // ARDUINO_HOST_WEB_SERVER_H
