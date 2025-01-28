#include "server.h"

#include <SDL3_net/SDL_net.h>

#include "event.h"
#include "level.h"
#include "log.h"
#include "other.h"
#include "request.h"
#include "response.h"
#include "sync.h"

Server::Server(std::string level_name, const uint16_t listen_port,
               uint max_clients)
    : socket(listen_port) {
    this->level_name = level_name;
    this->max_clients = max_clients;
}

Server::~Server() {}

// Run the server
void Server::Run() {
    Init();
    if (!Load()) return;
    GameLoop();
}

// Initialize the server
void Server::Init() {
    handler.RegisterDefault(std::function<void(Message *)>(
        std::bind(&Server::HandleUnknownMessage, this, std::placeholders::_1)));
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
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                log::info("quitting server ", GetCurrentTime());
                done = true;
            }
        }

        if (socket.Poll()) {
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
    log::debug("received unknown message type:",
               static_cast<int>(msg->GetMessageType()));
    // TODO: Should we output the content of the message?
}

// A player wants to join
void Server::HandleJoin(Message *msg) {
    auto join = msg->As<RequestJoinGame>();
    log::debug("client with name ", join->player_name, " requests join game");

    // Deny player if server full
    if (!(Player::instances.size() < this->max_clients)) {
        Packet pack;
        ResponseDenyJoin denial("server full");
        socket.Send(pack, handler.CurrentOrigin(), join->port);
        return;
    }

    // Allocate new player;
    auto new_player = new Player();

    log::debug("Added new client id# ", new_player->client_id);
    new_player->name = join->player_name;
    new_player->port = join->port;
    new_player->playaddr = handler.CurrentOrigin();

    // Partially prepare packet for new client
    Packet new_client_packet;
    ResponseAcceptJoin accept(new_player->client_id, new_player->team);
    new_client_packet.Append(accept);

    // Prepare join event msg for existing players
    Packet existing_client_pack;
    EventPlayerJoins new_join(new_player->client_id, new_player->team,
                              new_player->name);
    existing_client_pack.Append(new_join);

    // Iterate all existing players, add their information for the new
    // Player and send them the join event
    for (auto id_player : Player::instances) {
        // skip newly joined player
        if (id_player.first == new_player->client_id) continue;

        ResponsePlayerInformation client_info(id_player.second->client_id,
                                              id_player.second->name,
                                              id_player.second->team);
        new_client_packet.Append(client_info);

        socket.Send(existing_client_pack, id_player.second->playaddr,
                    id_player.second->port);
    }
    socket.Send(new_client_packet, handler.CurrentOrigin(), new_player->port);
}

// Client requests server information
void Server::HandleInfo(Message *msg) {
    auto info = msg->As<RequestGetServerInfo>();
    log::debug("client version ", (int)info->version, " requested information");

    // Send Info
    Packet pack;
    ResponseServerInformation response(SHIPZ_VERSION, Player::instances.size(),
                                       MAXPLAYERS, lvl.m_levelversion,
                                       lvl.m_filename);
    pack.Append(response);
    socket.Send(pack, handler.CurrentOrigin(), info->port);
    return;
}

// Setup message handling callbacks
void Server::SetupCallbacks() {
    this->handler.RegisterHandler(
        std::function<void(Message *)>(
            std::bind(&Server::HandleInfo, this, std::placeholders::_1)),
        ConstructHeader(MessageType::REQUEST, SERVER_INFO));
    this->handler.RegisterHandler(
        std::function<void(Message *)>(
            std::bind(&Server::HandleJoin, this, std::placeholders::_1)),
        ConstructHeader(MessageType::REQUEST, JOIN_GAME));
}