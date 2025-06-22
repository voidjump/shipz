#include <SDL3/SDL.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <cmdparser.hpp>
#include <fstream>
#include <iostream>

#include "client/client.h"
#include "client/gfx.h"
#include "common/globals.h"
#include "common/other.h"
#include "server/server.h"

void ConfigureParser(cli::Parser& parser) {
    parser.set_optional<bool>("s", "server", false, "Run as server");
    parser.set_optional<bool>("F", "fullscreen", false, "Run fullscreen (client)");
    parser.set_optional<std::string>("f", "filename", "rusty.json",
                                     "Which level to run");

    parser.set_optional<uint16_t>("l", "listen", PORT_CLIENT,
                                  "The port to listen on (client)");
    parser.set_optional<std::string>("a", "address", "localhost",
                                     "The server host address");
    parser.set_optional<std::string>("n", "nickname", "UnnamedPlayer",
                                     "Your nickname");
}

void RunMode(cli::Parser& parser) {
    auto run_as_server = parser.get<bool>("s");
    if (run_as_server) {
        auto filename = parser.get<std::string>("f");
        Server server(filename, PORT_SERVER, MAXPLAYERS);
        server.Run();
    } else {
        InitVid(parser.get<bool>("F"));
        auto listen_port = parser.get<uint16_t>("l");
        auto address = parser.get<std::string>("a");
        auto nickname = parser.get<std::string>("n");

        Client client(address.c_str(), nickname.c_str(), listen_port,
                      PORT_SERVER);
        client.Run();
    }
}

int main(int argc, char* argv[]) {
    cli::Parser parser(argc, argv);
    ConfigureParser(parser);
    parser.run_and_exit_if_error();

    InitSDL();

    try {
        RunMode(parser);
    } catch (const std::runtime_error& e) {
        logger::error("a fatal error occured:", e.what());
    }

    SDL_Quit();
    EndMessage();
}
