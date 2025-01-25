#include <SDL3/SDL.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <fstream>
#include <iostream>

#include "client.h"
#include "gfx.h"
#include "globals.h"
#include "menu.h"
#include "other.h"
#include "server.h"

// MAIN
// ///////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO: use some kind of modern CLI parsing lib
int main(int argc, char *argv[]) {
    bool run_server = false;  // is this the server or a client?
    bool cli_init = false;
    int error = 0, menu_result;

    char server_ip[16];
    char client_nick[13];
    char *level_filename;

    if (argc > 1) {
        if (strstr(argv[1], "server")) {
            if (argc < 3) {
                error = 10;
            } else {
                cli_init = true;
                run_server = true;
                level_filename = argv[2];
            }
        }
        if (strstr(argv[1], "client")) {
            if (argc < 4) {
                error = 10;
            } else {
                cli_init = true;
                run_server = false;
                strcpy(server_ip, argv[2]);
                strcpy(client_nick, argv[3]);
            }
        }
    }

    if (cli_init && error == 0) {
        InitSDL();
        if (run_server) {
            Server server(level_filename);
            server.Run();
        } else {
            InitVid();
            Client client(server_ip, client_nick, PORT_CLIENT, PORT_SERVER);
            client.Init();
            client.Run();
        }
    }

    SDL_Quit();
    EndMessage();
}
