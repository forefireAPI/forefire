#ifndef HTTP_COMMAND_SERVER_HPP
#define HTTP_COMMAND_SERVER_HPP

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <sstream>
#include <iostream>
#include <cstring>

// POSIX headers for sockets
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace http_command {

class HttpCommandServer {
public:
    // The callback takes the HTTP request body (the command text)
    // and returns a complete HTTP response as a string.
    using CommandCallback = std::function<std::string(const std::string&)>;

    HttpCommandServer() : server_fd(-1), running(false) {}

    ~HttpCommandServer() {
        stop();
    }

    // Opens a listening socket on the given address, e.g. "localhost:8000".
    // (The host part is ignored and INADDR_ANY is used.)
    bool listenOn(const std::string &address) {
        // Parse the address: get port number (after ':')
        size_t colon = address.find(':');
        if (colon == std::string::npos) {
            std::cerr << "Invalid address format. Expected host:port" << std::endl;
            return false;
        }
        int port = std::stoi(address.substr(colon + 1));

        // Create a TCP socket.
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
        addr.sin_addr.s_addr = INADDR_ANY;  // listen on any local interface
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
        // Run the accept loop in a new thread.
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

    // Set the callback function to execute a command.
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
            // For simplicity, handle one client at a time (synchronously).
            handleClient(client_fd);
            close(client_fd);
        }
    }

    // Updated client handler: reads the HTTP request, extracts the body,
    // calls the callback, and writes the complete response returned by the callback.
    void handleClient(int client_fd) {
        const int buf_size = 4096;
        char buffer[buf_size];
        int bytes_read = read(client_fd, buffer, buf_size - 1);
        if (bytes_read <= 0)
            return;
        buffer[bytes_read] = '\0';
        std::string request(buffer);
        // Look for the end of headers ("\r\n\r\n")
        size_t pos = request.find("\r\n\r\n");
        std::string body;
        if (pos != std::string::npos) {
            body = request.substr(pos + 4);
        }
        // Call the callback to produce a complete HTTP response.
        std::string complete_response;
        if (callback) {
            complete_response = callback(body);
        } else {
            complete_response = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
        }
        // Write the response exactly as returned by the callback.
        write(client_fd, complete_response.c_str(), complete_response.size());
    }

    int server_fd;
    std::atomic<bool> running;
    std::thread server_thread;
    CommandCallback callback;
};

} // namespace http_command

#endif // HTTP_COMMAND_SERVER_HPP