#include "server.h"

#include <SDL3_net/SDL_net.h>
#include "log.h"
#include "level.h"

Server::Server(std::string level_name, const uint16_t listen_port)
    : socket(listen_port) {
    this->level_name = level_name;
}

Server::~Server() {

}

// Run the server
void Server::Run() {
	Init();
    if(!Load()) return;
    GameLoop();
}

// Initialize the server
void Server::Init() {
}

// Perform level & asset loading
bool Server::Load() {
	lvl.SetFile(level_name);
	log::info("loading level ", level_name);
    if (!lvl.Load()) {
        log::error("failed to load level");
        return false;
    }
	return true;
}

// Run the game loop; Start listening
void Server::GameLoop() {

}