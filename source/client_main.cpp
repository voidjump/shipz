#include <SDL3/SDL.h>
#include <cmdparser.hpp>
#include <iostream>

#include "client/client.h"
#include "client/gfx.h"
#include "common/globals.h"
#include "common/other.h"

void ConfigureClientParser(cli::Parser& parser) {
    parser.set_optional<bool>("F", "fullscreen", false, "Run fullscreen");
    parser.set_optional<uint16_t>("l", "listen", PORT_CLIENT,
                                  "The port to listen on");
    parser.set_optional<std::string>("a", "address", "localhost",
                                     "The server host address");
    parser.set_optional<std::string>("n", "nickname", "UnnamedPlayer",
                                     "Your nickname");
}

int main(int argc, char* argv[]) {
    cli::Parser parser(argc, argv);
    ConfigureClientParser(parser);
    parser.run_and_exit_if_error();

    InitSDL();

    try {
        InitVid(parser.get<bool>("F"));
        auto listen_port = parser.get<uint16_t>("l");
        auto address = parser.get<std::string>("a");
        auto nickname = parser.get<std::string>("n");

        Client client(address.c_str(), nickname.c_str(), listen_port, PORT_SERVER);
        client.Run();
    } catch (const std::runtime_error& e) {
        logger::error("a fatal error occured:", e.what());
    }

    SDL_Quit();
    EndMessage();
}
