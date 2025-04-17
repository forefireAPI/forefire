/**
 * @file HttpCommandServer.hpp
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#ifndef HTTP_COMMAND_SERVER_HPP
#define HTTP_COMMAND_SERVER_HPP

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <sstream>
#include <iostream>
#include <cstring>
#include <sys/stat.h>
#include <dirent.h>

// POSIX headers for sockets
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace http_command {

    class HttpCommandServer {
    public:
        // The callback takes the command string (after the "ff:" prefix)
        // and returns a complete HTTP response as a string.
        using CommandCallback = std::function<std::string(const std::string&)>;

        HttpCommandServer() : server_fd(-1), running(false) {}

        ~HttpCommandServer() {
            stop();
        }

        // Opens a listening socket on the given address (e.g. "localhost:8000").
        bool listenOn(const std::string &address) {
            size_t colon = address.find(':');
            if (colon == std::string::npos) {
                std::cerr << "Invalid address format. Expected host:port" << std::endl;
                return false;
            }
            int port = std::stoi(address.substr(colon + 1));

            server_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (server_fd < 0) {
                perror("socket");
                return false;
            }
            int opt = 1;
            setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

            sockaddr_in addr;
            std::memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = htons(port);
            if (::bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
                perror("bind");
                return false;
            }
            if (::listen(server_fd, 10) < 0) {
                perror("listen");
                return false;
            }
            running = true;
            server_thread = std::thread(&HttpCommandServer::acceptLoop, this);
            std::cout << "HTTP command server listening on port " << port << std::endl;
            return true;
        }

        // Stops the server.
        void stop() {
            running = false;
            if (server_fd >= 0) {
                close(server_fd);
                server_fd = -1;
            }
            if (server_thread.joinable()) {
                server_thread.join();
            }
        }

        // Set the callback function for processing ff: commands.
        void setCallback(CommandCallback cb) {
            callback = cb;
        }

    private:
        // Main loop: accept incoming connections.
        void acceptLoop() {
            while (running) {
                sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
                if (client_fd < 0) {
                    if (running) {
                        perror("accept");
                    }
                    continue;
                }
                handleClient(client_fd);
                close(client_fd);
            }
        }

        // Handle a client connection by parsing the request.
        void handleClient(int client_fd) {
            const int buf_size = 4096;
            char buffer[buf_size];
            int bytes_read = read(client_fd, buffer, buf_size - 1);
            if (bytes_read <= 0)
                return;
            buffer[bytes_read] = '\0';
            std::string request(buffer);

            // Parse the request line (e.g., "GET /uri HTTP/1.1" or "POST /uri HTTP/1.1").
            std::istringstream requestStream(request);
            std::string method, uri, version;
            requestStream >> method >> uri >> version;

            std::string complete_response;
            // Handle POST requests: look for an ff: command in the body.
            if (method == "POST") {
                size_t pos = request.find("\r\n\r\n");
                std::string body;
                if (pos != std::string::npos) {
                    body = request.substr(pos + 4);
                }
                if (body.size() > 3 && (body.substr(0, 3) == "ff:" || body.substr(0, 3) == "FF:")) {
                    std::string fireCommand = body.substr(3);
                    std::string result = callback(fireCommand);
                    complete_response = buildResponse("200 OK", "text/plain; charset=UTF-8", result);
                } else {
                    complete_response = serveFileOrDirectory(uri);
                }
            }
            // Handle GET requests.
            else {
                if (uri.size() > 4 && (uri.substr(0, 4) == "/ff:" || uri.substr(0, 4) == "/FF:")) {
                    std::string fireCommand = uri.substr(4);
                    std::string result = callback(fireCommand);
                    complete_response = buildResponse("200 OK", "text/plain; charset=UTF-8", result);
                } else {
                    complete_response = serveFileOrDirectory(uri);
                }
            }
            write(client_fd, complete_response.data(), complete_response.size());
        }

        // Check whether a file exists.
        bool fileExists(const std::string &path) {
            std::ifstream file(path, std::ios::binary);
            return file.good();
        }

        // Check whether the given path is a directory.
        bool isDirectory(const std::string &path) {
            struct stat st;
            if (stat(path.c_str(), &st) == 0) {
                return S_ISDIR(st.st_mode);
            }
            return false;
        }

        // Determine the Content-Type based on the file extension.
        std::string determineContentType(const std::string &path) {
            size_t dotPos = path.find_last_of('.');
            if (dotPos != std::string::npos) {
                std::string ext = path.substr(dotPos);
                if (ext == ".html" || ext == ".htm")
                    return "text/html; charset=UTF-8";
                else if (ext == ".png" || ext == ".PNG")
                    return "image/png";
                else if (ext == ".jpg" || ext == ".jpeg" ||
                         ext == ".JPG" || ext == ".JPEG")
                    return "image/jpeg";
                else if (ext == ".json")
                    return "application/json";
                else if (ext == ".css")
                    return "text/css; charset=UTF-8";
                else if (ext == ".js")
                    return "application/javascript; charset=UTF-8";
            }
            return "application/octet-stream";
        }

        // Build an HTTP response from status, content type, and body.
        std::string buildResponse(const std::string &status,
                                  const std::string &contentType,
                                  const std::string &body) {
            std::ostringstream responseStream;
            std::time_t now = std::time(nullptr);
            char dateBuf[100];
            std::tm *gmt = std::gmtime(&now);
            std::strftime(dateBuf, sizeof(dateBuf), "%a, %d %b %Y %H:%M:%S GMT", gmt);

            responseStream << "HTTP/1.0 " << status << "\r\n"
                           << "Date: " << dateBuf << "\r\n"
                           << "Connection: close\r\n"
                           << "Content-Type: " << contentType << "\r\n"
                           << "Content-Length: " << body.size() << "\r\n"
                           << "\r\n"
                           << body;
            return responseStream.str();
        }

        std::string buildSimpleResponse(const std::string &status,
                                        const std::string &message) {
            return buildResponse(status, "text/plain; charset=UTF-8", message);
        }

        // Build a simple HTML directory listing.
        std::string buildDirectoryListing(const std::string &dirPath) {
            std::ostringstream listing;
            listing << "<!DOCTYPE html>\n<html>\n<head>\n"
                    << "<meta charset=\"utf-8\">\n"
                    << "<title>Directory Listing for " << dirPath << "</title>\n"
                    << "</head>\n<body>\n"
                    << "<h1>Directory Listing for " << dirPath << "</h1>\n"
                    << "<ul>\n";
            DIR *dir = opendir(dirPath.c_str());
            if (dir) {
                struct dirent *entry;
                while ((entry = readdir(dir)) != nullptr) {
                    std::string name(entry->d_name);
                    if (name == "." || name == "..")
                        continue;
                    std::string link = dirPath;
                    if (link.back() != '/')
                        link.push_back('/');
                    link += name;
                    listing << "<li><a href=\"/" << link << "\">" << name << "</a></li>\n";
                }
                closedir(dir);
            } else {
                listing << "<li>Unable to open directory.</li>\n";
            }
            listing << "</ul>\n</body>\n</html>\n";
            return listing.str();
        }

        // Serve a static file or a directory listing based on the request URI.
        std::string serveFileOrDirectory(const std::string &uri) {
            std::string path = uri;
            size_t queryPos = path.find('?');
            if (queryPos != std::string::npos)
                path = path.substr(0, queryPos);
            if (!path.empty() && path[0] == '/')
                path = path.substr(1);

            // For an empty path, try current directory's index.html;
            // if not available, check the FOREFIREHOME environment variable.
            if (path.empty()) {
                if (fileExists("index.html"))
                    path = "index.html";
                else if (const char* ffHome = std::getenv("FOREFIREHOME")) {
                    std::string altIndex = std::string(ffHome) + "/tools/htdocs/index.html";
                    if (fileExists(altIndex))
                        path = altIndex;
                }
            }

            if (isDirectory(path)) {
                if (uri.empty() || uri.back() != '/') {
                    std::string location = "/" + path + "/";
                    return buildResponse("301 Moved Permanently", "text/plain; charset=UTF-8",
                                           "Redirecting to " + location);
                }
                std::string body = buildDirectoryListing(path);
                return buildResponse("200 OK", "text/html; charset=UTF-8", body);
            } else {
                if (!fileExists(path)) {
                    if (const char* ffHome = std::getenv("FOREFIREHOME")) {
                        
                        std::string altPath = std::string(ffHome) +"/tools/htdocs/"+ path;
                        if (fileExists(altPath))
                            path = altPath;
                    }
                }
                if (fileExists(path)) {
                    std::ifstream file(path, std::ios::in | std::ios::binary);
                    if (!file) {
                        return buildSimpleResponse("404 Not Found", "Unable to open file: " + path);
                    }
                    std::ostringstream contents;
                    contents << file.rdbuf();
                    std::string body = contents.str();
                    std::string contentType = determineContentType(path);
                    return buildResponse("200 OK", contentType, body);
                }
                return buildSimpleResponse("404 Not Found", "File not found: " + path);
                }
            }

        int server_fd;
        std::atomic<bool> running;
        std::thread server_thread;
        CommandCallback callback;
    };

} // namespace http_command

#endif // HTTP_COMMAND_SERVER_HPP