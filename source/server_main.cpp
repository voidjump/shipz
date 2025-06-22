#include <SDL3/SDL.h>
#include <cmdparser.hpp>
#include <iostream>

#include "common/globals.h"
#include "common/other.h"
#include "server/server.h"

void ConfigureServerParser(cli::Parser& parser) {
    parser.set_optional<std::string>("f", "filename", "rusty.json",
                                     "Which level to run");
}

int main(int argc, char* argv[]) {
    cli::Parser parser(argc, argv);
    ConfigureServerParser(parser);
    parser.run_and_exit_if_error();

    InitSDL();

    try {
        auto filename = parser.get<std::string>("f");
        Server server(filename, PORT_SERVER, MAXPLAYERS);
        server.Run();
    } catch (const std::runtime_error& e) {
        log::error("a fatal error occured:", e.what());
    }

    SDL_Quit();
    EndMessage();
}
