#include "server.h"

#include <SDL3_net/SDL_net.h>
#include "log.h"
#include "level.h"
#include "request.h"
#include "response.h"
#include "event.h"
#include "sync.h"
#include "other.h"

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
    handler.RegisterDefault(
        std::function<void(Message*)>(
            std::bind(&Server::HandleUnknownMessage, this, std::placeholders::_1)
        )
    );
    SetupCallbacks();
    // Setup chat console
    console.SetHeight(0);
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
    log::info("starting server ", GetCurrentTime());
    SDL_Event event;
    while(!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                log::info("quitting server ", GetCurrentTime());
                done = true;
            }
        }
        
        if(socket.Poll()) {
            log::debug("received a packet");
            auto recieved_packet = socket.GetPacket();
            handler.HandlePacket(*recieved_packet);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// MESSAGE HANDLERS
///////////////////////////////////////////////////////////////////////////////

// Handle an unknown message
void Server::HandleUnknownMessage(Message *msg) {
    log::debug("received unknown message type:", static_cast<int>(msg->GetMessageType()));
    // TODO: Should we output the content of the message?
}

// A player wants to join 
void Server::HandleJoin(Message *msg) {
    auto join = msg->As<EventPlayerJoins>(); 
}

// A player wants to join 
void Server::HandleInfo(Message *msg) {
    auto info = msg->As<RequestGetServerInfo>(); 
    int foo = info->version;
    // log::debug("client version ", (int)info->version, " requested information");

    // Send Info
    Packet pack;
    ResponseServerInformation response(SHIPZ_VERSION,
                                       Player::instances.size(),
                                       MAXPLAYERS,
                                       lvl.m_levelversion,
                                       lvl.m_filename );
    pack.Append(response);
    socket.Send(pack, handler.CurrentOrigin(), PORT_CLIENT);
    return;
}

// Setup message handling callbacks
void Server::SetupCallbacks() {
    this->handler.RegisterHandler(
        std::function<void(Message*)>(
            std::bind(&Server::HandleInfo, this, std::placeholders::_1)
        ),
        SERVER_INFO
    );
    // this->handler.RegisterHandler(
    //     std::function<void(Message&)>(
    //         std::bind(&Server::HandleJoin, this, std::placeholders::_1)
    //     ),
    //     PLAYER_JOINS
    // );
}