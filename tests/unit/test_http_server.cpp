/**
 * @file test_http_server.cpp
 * @brief What the HTTP command server does today, pinned before it is changed.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 *
 * The server is due for hardening, and nothing tested it. These cases record
 * the behaviour the web console in tools/htdocs depends on, so that a change
 * to request parsing or path resolution shows up here rather than as a blank
 * map in someone's browser.
 *
 * Two things the console needs, both covered below:
 *
 *   - POST / with a body of "ff:<command>" reaches the interpreter. That is
 *     the whole UI-to-engine channel (forefireGUI.js sendCommand).
 *   - Files are served relative to the working directory. The console issues
 *     plot[filename=fuel.png], ForeFire writes fuel.png next to the case, and
 *     the browser then fetches /fuel.png as a static file. Confining the
 *     document root without accounting for this would break every overlay.
 *
 * These are characterisation tests: they say what happens now, not what ought
 * to. Where current behaviour is a known limitation it is marked, so that a
 * case flipping is read as "the fix landed" rather than "something broke".
 */

#include "doctest/doctest.h"

#include "HttpCommandServer.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

using http_command::HttpCommandServer;

namespace {

/** Sends one raw request and reads the whole response.
 *
 * The server closes the connection after replying, so reading to EOF is
 * enough — no need to parse Content-Length on this side. */
std::string request(int port, const std::string& raw) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    REQUIRE(fd >= 0);

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    REQUIRE(inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) == 1);

    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(fd);
        return "";
    }
    if (write(fd, raw.data(), raw.size()) < 0) {
        close(fd);
        return "";
    }

    std::string response;
    char buffer[4096];
    ssize_t n;
    while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
        response.append(buffer, static_cast<size_t>(n));
    }
    close(fd);
    return response;
}

/** What the callback was last handed, so a test can check the "ff:" prefix is
 *  stripped rather than only that a reply came back. */
std::string& lastCommand() {
    static std::string command;
    return command;
}

/** One server, started once and deliberately never stopped.
 *
 * stop() cannot be called: it sets running=false, closes the listening socket
 * and then joins the accept thread — but closing a socket does not reliably
 * wake a thread blocked in accept() on Linux, so the join never returns and
 * the process hangs. Fixing that needs shutdown() or a self-pipe, which is a
 * change to server behaviour and belongs with the pending hardening work
 * rather than with a test that is supposed to be purely additive.
 *
 * Leaking it is safe here: the object is never destroyed, so the joinable
 * std::thread member never runs its destructor, and the thread dies with the
 * process. Sharing one server across the suite also keeps the port scan to a
 * single pass.
 *
 * The address is given as 127.0.0.1 deliberately. The host half is currently
 * discarded by listenOn, which binds INADDR_ANY regardless — writing it this
 * way means these tests keep passing once the address is honoured. */
int serverPort() {
    static int port = [] {
        HttpCommandServer* server = new HttpCommandServer();  // never deleted
        server->setCallback([](const std::string& command) {
            lastCommand() = command;
            return "callback saw: " + command;
        });
        // Nothing reports the bound port back, so try until one takes.
        for (int candidate = 18730; candidate < 18790; candidate++) {
            char address[32];
            std::snprintf(address, sizeof(address), "127.0.0.1:%d", candidate);
            if (server->listenOn(address)) return candidate;
        }
        return 0;
    }();
    return port;
}

/** Writes a file into the working directory and removes it afterwards.
 *
 * The working directory is where the server looks, and where ForeFire writes
 * the PNGs the console then requests. */
class ScratchFile {
public:
    ScratchFile(const std::string& name, const std::string& contents)
        : path(name) {
        std::ofstream out(path.c_str(), std::ios::binary);
        out << contents;
    }
    ~ScratchFile() { std::remove(path.c_str()); }

private:
    ScratchFile(const ScratchFile&);
    ScratchFile& operator=(const ScratchFile&);
    std::string path;
};

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

} /* anonymous namespace */

TEST_SUITE("http server") {

TEST_CASE("the server binds and accepts a connection") {
    REQUIRE_MESSAGE(serverPort() != 0,
                    "no free port in the test range; nothing else is testable");

    const std::string response =
        request(serverPort(), "GET /nothing-here HTTP/1.1\r\n\r\n");
    CHECK_FALSE(response.empty());
}

TEST_CASE("a file in the working directory is served") {
    // This is the path the web console relies on for plot overlays: ForeFire
    // writes <layer>.png beside the case, the browser fetches /<layer>.png.
    REQUIRE(serverPort() != 0);
    lastCommand().clear();
    ScratchFile file("ff_test_overlay.txt", "overlay-body");

    const std::string response =
        request(serverPort(), "GET /ff_test_overlay.txt HTTP/1.1\r\n\r\n");

    CHECK(contains(response, "200 OK"));
    CHECK(contains(response, "overlay-body"));
}

TEST_CASE("a missing file returns 404") {
    REQUIRE(serverPort() != 0);
    lastCommand().clear();

    const std::string response =
        request(serverPort(), "GET /definitely_absent_file HTTP/1.1\r\n\r\n");

    CHECK(contains(response, "404 Not Found"));
}

TEST_CASE("a POST body prefixed with ff: reaches the command callback") {
    // forefireGUI.js sendCommand() posts to "/" with body "ff:<command>".
    // This is the entire UI-to-engine channel.
    REQUIRE(serverPort() != 0);
    lastCommand().clear();

    const std::string body = "ff:getParameter[ISOdate]";
    std::string raw = "POST / HTTP/1.1\r\n";
    raw += "Content-Type: text/plain\r\n";
    raw += "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n";
    raw += body;

    const std::string response = request(serverPort(), raw);

    CHECK(contains(response, "200 OK"));
    CHECK(contains(response, "callback saw: getParameter[ISOdate]"));
    // The "ff:" prefix must be stripped before the interpreter sees it.
    CHECK(lastCommand() == "getParameter[ISOdate]");
}

TEST_CASE("a GET uri prefixed with /ff: reaches the command callback") {
    REQUIRE(serverPort() != 0);
    lastCommand().clear();

    const std::string response =
        request(serverPort(), "GET /ff:print[] HTTP/1.1\r\n\r\n");

    CHECK(contains(response, "200 OK"));
    CHECK(contains(response, "callback saw: print[]"));
    CHECK(lastCommand() == "print[]");
}

TEST_CASE("an ordinary file request does not reach the command callback") {
    // Static assets and commands share one endpoint, so the split between
    // them is worth pinning: js/style/png requests must not be interpreted.
    REQUIRE(serverPort() != 0);
    lastCommand().clear();
    ScratchFile file("ff_test_asset.css", "body{}");

    const std::string response =
        request(serverPort(), "GET /ff_test_asset.css HTTP/1.1\r\n\r\n");

    CHECK(contains(response, "200 OK"));
    CHECK(lastCommand().empty());
}

TEST_CASE("a request comfortably inside one buffer round-trips intact") {
    // handleClient currently reads a single fixed-size buffer rather than
    // consulting Content-Length, so only requests well under that size are
    // guaranteed. This pins a size the console can rely on today; raising it
    // is part of the pending hardening work.
    REQUIRE(serverPort() != 0);
    lastCommand().clear();

    const std::string argument(1000, 'x');
    const std::string body = "ff:setParameter[note=" + argument + "]";
    std::string raw = "POST / HTTP/1.1\r\n";
    raw += "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n";
    raw += body;

    const std::string response = request(serverPort(), raw);

    CHECK(contains(response, "200 OK"));
    CHECK(contains(lastCommand(), argument));
    CHECK(lastCommand().size() == body.size() - 3);
}

} /* TEST_SUITE */
