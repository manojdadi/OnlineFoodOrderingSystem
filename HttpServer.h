// HttpServer.h - a very small web server written with plain sockets (no external library).
// It lets the browser (frontend) talk to our C++ classes (backend).
// OOP concepts: Encapsulation (all socket details are hidden inside this class),
//               std::function callbacks, multithreading with std::thread and std::mutex
#pragma once
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include "Exceptions.h"
#include "Utils.h"

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  typedef SOCKET socket_t;
  #define CLOSE_SOCKET closesocket
#else
  #include <arpa/inet.h>
  #include <csignal>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <sys/time.h>
  #include <unistd.h>
  typedef int socket_t;
  #define CLOSE_SOCKET close
#endif

struct HttpRequest {
    std::string method;                              // GET or POST
    std::string path;                                // e.g. /api/menu
    std::map<std::string, std::string> params;       // form fields and ?query=values
    std::map<std::string, std::string> headers;      // header names in lower case

    std::string param(const std::string& key) const {
        auto it = params.find(key);
        return it == params.end() ? "" : it->second;
    }
    std::string header(const std::string& key) const {
        auto it = headers.find(key);
        return it == headers.end() ? "" : it->second;
    }
    // Reads a whole number from the form, e.g. itemId or quantity
    int intParam(const std::string& key) const {
        try {
            size_t used = 0;
            std::string value = util::trim(param(key));
            int number = std::stoi(value, &used);
            if (used != value.size()) throw std::invalid_argument(key);
            return number;
        } catch (const std::exception&) {
            throw InvalidInputException("'" + key + "' must be a number");
        }
    }
};

struct HttpResponse {
    int status = 200;
    std::string contentType = "application/json";
    std::string body;
};

class HttpServer {
public:
    typedef std::function<HttpResponse(const HttpRequest&)> Handler;

private:
    std::map<std::string, Handler> routes;           // "GET /api/menu" -> function
    std::string staticDir;
    std::mutex handlerMutex;                         // only one request touches the data at a time

    static void parseParams(const std::string& text, std::map<std::string, std::string>& out) {
        for (const std::string& pair : util::split(text, '&')) {
            if (pair.empty()) continue;
            size_t eq = pair.find('=');
            if (eq == std::string::npos) out[util::urlDecode(pair)] = "";
            else out[util::urlDecode(pair.substr(0, eq))] = util::urlDecode(pair.substr(eq + 1));
        }
    }

    static std::string statusText(int status) {
        switch (status) {
            case 200: return "OK";
            case 400: return "Bad Request";
            case 401: return "Unauthorized";
            case 402: return "Payment Required";
            case 403: return "Forbidden";
            case 404: return "Not Found";
            default: return "Error";
        }
    }

    static std::string contentTypeOf(const std::string& path) {
        size_t dot = path.rfind('.');
        std::string ext = dot == std::string::npos ? "" : path.substr(dot);
        if (ext == ".html") return "text/html; charset=utf-8";
        if (ext == ".css") return "text/css; charset=utf-8";
        if (ext == ".js") return "application/javascript; charset=utf-8";
        if (ext == ".png") return "image/png";
        if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
        if (ext == ".svg") return "image/svg+xml";
        return "application/octet-stream";
    }

    HttpResponse errorResponse(int status, const std::string& message) const {
        HttpResponse response;
        response.status = status;
        response.body = "{\"success\":false,\"message\":" + util::jsonStr(message) + "}";
        return response;
    }

    // Sends index.html, style.css, app.js ... from the frontend folder
    HttpResponse serveFile(std::string path) const {
        if (path == "/") path = "/index.html";
        if (staticDir.empty() || path.find("..") != std::string::npos) return errorResponse(404, "Not found");
        std::ifstream file(staticDir + path, std::ios::binary);
        if (!file) return errorResponse(404, "Not found");
        std::ostringstream content;
        content << file.rdbuf();
        HttpResponse response;
        response.contentType = contentTypeOf(path);
        response.body = content.str();
        return response;
    }

    HttpResponse dispatch(const HttpRequest& request) {
        auto it = routes.find(request.method + " " + request.path);
        if (it == routes.end()) {
            if (request.method == "GET" && request.path.rfind("/api/", 0) != 0) return serveFile(request.path);
            return errorResponse(404, "No such API: " + request.path);
        }
        std::lock_guard<std::mutex> lock(handlerMutex);
        try {
            return it->second(request);
        } catch (const FoodException& e) {               // our own exceptions carry a status code
            return errorResponse(e.getHttpStatus(), e.what());
        } catch (const std::exception& e) {              // anything unexpected
            std::cerr << "Server error: " << e.what() << "\n";
            return errorResponse(500, "Something went wrong on the server");
        }
    }

    static bool sendAll(socket_t client, const std::string& data) {
        size_t sent = 0;
        while (sent < data.size()) {
            int n = static_cast<int>(send(client, data.data() + sent, static_cast<int>(data.size() - sent), 0));
            if (n <= 0) return false;
            sent += static_cast<size_t>(n);
        }
        return true;
    }

    void handleClient(socket_t client) {
        // 1. Read the raw request text
        std::string raw;
        char buffer[4096];
        size_t headerEnd = std::string::npos;
        size_t contentLength = 0;
        while (raw.size() < 1024 * 1024) {
            int n = static_cast<int>(recv(client, buffer, sizeof(buffer), 0));
            if (n <= 0) break;
            raw.append(buffer, static_cast<size_t>(n));
            if (headerEnd == std::string::npos) {
                headerEnd = raw.find("\r\n\r\n");
                if (headerEnd != std::string::npos) {
                    std::string lower = util::toLower(raw.substr(0, headerEnd));
                    size_t pos = lower.find("content-length:");
                    if (pos != std::string::npos) {
                        try { contentLength = static_cast<size_t>(std::stoul(lower.substr(pos + 15))); }
                        catch (const std::exception&) { contentLength = 0; }
                    }
                }
            }
            if (headerEnd != std::string::npos && raw.size() >= headerEnd + 4 + contentLength) break;
        }
        if (headerEnd == std::string::npos) { CLOSE_SOCKET(client); return; }

        // 2. Break it into method, path, headers and form fields
        HttpRequest request;
        std::istringstream head(raw.substr(0, headerEnd));
        std::string line, target;
        std::getline(head, line);
        std::istringstream first(line);
        first >> request.method >> target;
        while (std::getline(head, line)) {
            size_t colon = line.find(':');
            if (colon != std::string::npos)
                request.headers[util::toLower(util::trim(line.substr(0, colon)))] = util::trim(line.substr(colon + 1));
        }
        size_t question = target.find('?');
        request.path = target.substr(0, question);
        if (question != std::string::npos) parseParams(target.substr(question + 1), request.params);
        if (request.method == "POST") parseParams(raw.substr(headerEnd + 4, contentLength), request.params);

        // 3. Run the matching handler and send the answer back
        HttpResponse response = dispatch(request);
        std::string out = "HTTP/1.1 " + std::to_string(response.status) + " " + statusText(response.status) + "\r\n" +
                          "Content-Type: " + response.contentType + "\r\n" +
                          "Content-Length: " + std::to_string(response.body.size()) + "\r\n" +
                          "Cache-Control: no-store\r\nConnection: close\r\n\r\n" + response.body;
        sendAll(client, out);
        CLOSE_SOCKET(client);
    }

public:
    void get(const std::string& path, Handler handler) { routes["GET " + path] = handler; }
    void post(const std::string& path, Handler handler) { routes["POST " + path] = handler; }
    void serveStaticFiles(const std::string& directory) { staticDir = directory; }

    // Starts the server. This function runs until the program is stopped (Ctrl + C).
    // publicAccess = false : only this computer can connect (safe default for running on a laptop)
    // publicAccess = true  : any computer can connect (used when hosted online, e.g. on Render)
    void listen(int port, bool publicAccess = false) {
#ifdef _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) throw std::runtime_error("Could not start Winsock");
#else
        std::signal(SIGPIPE, SIG_IGN);                   // do not crash if the browser closes early
#endif
        socket_t server = socket(AF_INET, SOCK_STREAM, 0);
        int yes = 1;
        setsockopt(server, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof(yes));

        sockaddr_in address;
        std::memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(publicAccess ? INADDR_ANY : INADDR_LOOPBACK);
        address.sin_port = htons(static_cast<unsigned short>(port));

        if (bind(server, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0)
            throw std::runtime_error("Port " + std::to_string(port) + " is already in use. Close the other program or pass another port.");
        if (::listen(server, 16) != 0) throw std::runtime_error("Could not listen on the port");

        std::cout << "\nFoodExpress server is running.\nOpen this in your browser:  http://localhost:" << port
                  << "\nPress Ctrl + C to stop.\n" << std::endl;

        while (true) {
            socket_t client = accept(server, nullptr, nullptr);
#ifdef _WIN32
            if (client == INVALID_SOCKET) continue;
            DWORD timeout = 5000;
            setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
#else
            if (client < 0) continue;
            timeval timeout;
            timeout.tv_sec = 5; timeout.tv_usec = 0;
            setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
#endif
            // Each browser connection gets its own thread
            std::thread([this, client]() { handleClient(client); }).detach();
        }
    }
};
