#include "server.h"

#include <SDL3_net/SDL_net.h>

#include "event.h"
#include "level.h"
#include "log.h"
#include "other.h"
#include "request.h"
#include "response.h"
#include "session.h"
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
    handler.RegisterDefault(std::function<void(MessagePtr, ShipzSession*)>(
        std::bind(&Server::HandleUnknownMessage, this, std::placeholders::_1,
                  std::placeholders::_2)));
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

        // TODO: Stick this in some sort of timer
        PurgeStaleSessions();

        if (socket.Poll()) {
            auto recieved_packet = socket.GetPacket();
            auto session_id = recieved_packet->SessionID();
            log::debug("received a packet for session ", session_id);

            if (ShipzSession::IsNoneID(session_id)) {
                // This is not a known session, so we directly handle the
                // messages in the packet
                this->handler.HandlePacket(*recieved_packet);
            } else if (ShipzSession::IsActiveID(session_id)) {
                // Let the session's manager handle the packet
                // (Could be ack message, session state etc.)
                ShipzSession::GetSessionById(session_id)
                    ->manager->HandleReceivedPacket(*recieved_packet);
            } else {
                log::debug("dropped packed due to invalid session");
            }
        }

        // Handle messages from clients
        HandleInboundMessages();

        // TODO: Game update functions
        // Updating object statuses, detecting collisions,
        // state machines etc.

        // Send messages to clients
        SendOutboundMessages();
    }
}

// Call handlers on all inbound queues
void Server::HandleInboundMessages() {
    for (auto session : active_sessions) {
        for (auto msg : session->manager->Read()) {
            this->handler.HandleMessage(msg, session);
        }
    }
}

void Server::SendOutboundMessages() {
    // Send out all inbound traffic
    for (auto session : active_sessions) {
        if(session->LastSendGreaterThan(80)) {
            auto packet = session->manager->CraftSendPacket();
            if (packet != nullptr) {
                this->socket.Send(*packet, session->endpoint, session->port);
                session->SendTick();
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// MESSAGE HANDLERS
///////////////////////////////////////////////////////////////////////////////

// Handle an unknown message
void Server::HandleUnknownMessage(MessagePtr msg, ShipzSession* session) {
    log::debug("received unknown message type:",
               static_cast<int>(msg->GetMessageType()),
               " session:", session->session_id);
}

// A player wants to join
void Server::HandleJoin(MessagePtr msg, ShipzSession* session) {
    if (session == nullptr) {
        // Not allowed without session
        return;
    }
    auto join = msg->As<RequestJoinGame>();
    log::debug("client with name ", join->player_name, " requests join game");

    // Deny player if server full
    if (!(Player::instances.size() < this->max_clients)) {
        auto deny = std::make_shared<ResponseDenyJoin>("server full");
        session->manager->Write(deny);
        return;
    }

    // Allocate new player;
    auto new_player = new Player(session);

    log::debug("Added new client id# ", new_player->player_id);
    new_player->name = join->player_name;

    session->Write<ResponseAcceptJoin>(new_player->player_id, new_player->team);

    auto new_join_event = std::make_shared<EventPlayerJoins>(
        new_player->player_id, new_player->team, new_player->name);

    // Notify all players and send new player their information
    for (auto id_player : Player::instances) {
        // skip newly joined player
        if (id_player.first == new_player->player_id) continue;

        session->Write<ResponsePlayerInformation>(id_player.second->player_id,
                                                  id_player.second->name,
                                                  id_player.second->team);

        id_player.second->session->manager->Write(new_join_event);
    }
}

// Client requests server information
void Server::HandleInfo(MessagePtr msg, ShipzSession* session) {
    if (session == nullptr) {
        // Not allowed without session
        return;
    }
    auto info = msg->As<RequestGetServerInfo>();
    log::debug("client version ", (int)info->version, " requested information");

    session->Write<ResponseServerInformation>(
        SHIPZ_VERSION, Player::instances.size(), MAXPLAYERS, lvl.m_levelversion,
        lvl.m_filename);
    return;
}

// Create a session for address and port:
ShipzSession* Server::CreateSessionForClient(SDLNet_Address* addr,
                                             uint16_t port) {
    // TODO: Refuse to create session for already existing endpoint
    if (ShipzSession::Exists(addr, port)) {
        log::error(
            "Refusing to create new session; Addr and port already in use");
        return nullptr;
    }

    ShipzSession* new_session = new ShipzSession(addr, port);
    if (new_session == nullptr) {
        log::error("Failed to create new session instance");
        return nullptr;
    }
    active_sessions.push_back(new_session);
    return new_session;
}

// Client wants to initiate a session
// If the endpoint belonging to the player doens't have a session lining
// up yet, create a session. Otherwise, the session request is simply
// ignored.
void Server::HandleCreateSession(MessagePtr msg, ShipzSession* session) {
    if (session != nullptr) {
        log::error("ignoring request to create session for existing session");
        return;
    }

    auto info = msg->As<SessionRequestSession>();
    log::debug("client version ", (int)info->version, " requests new session");

    auto new_session =
        CreateSessionForClient(this->handler.CurrentOrigin(), info->port);
    SessionProvideSession provide_session_message(new_session->session_id);
    if (new_session == nullptr) {
        log::error("Session request dropped");
        return;
    }

    // Send Info; Note this has no session header
    Packet pack;
    pack.Append(provide_session_message);
    socket.Send(pack, handler.CurrentOrigin(), info->port);
    return;
}

// Purge all stale sessions older than MAX_SESSION_AGE
void Server::PurgeStaleSessions() {
    uint64_t time = SDL_GetTicks();
    for (auto it = active_sessions.begin(); it != active_sessions.end();) {
        if (time - (*it)->last_active > MAX_SESSION_AGE) {
            log::info("Dropping stale session ", (*it)->session_id);
            delete (*it);
            it = active_sessions.erase(it);
        } else {
            ++it;
        }
    }
}

// Setup message handling callbacks
void Server::SetupCallbacks() {
    // Client wants to initiate a session
    this->handler.RegisterHandler(
        std::function<void(MessagePtr, ShipzSession*)>(
            std::bind(&Server::HandleCreateSession, this, std::placeholders::_1,
                      std::placeholders::_2)),
        ConstructHeader(MessageType::SESSION, REQUEST_SESSION));

    // Client requests server information
    this->handler.RegisterHandler(
        std::function<void(MessagePtr, ShipzSession*)>(
            std::bind(&Server::HandleInfo, this, std::placeholders::_1,
                      std::placeholders::_2)),
        ConstructHeader(MessageType::REQUEST, SERVER_INFO));

    // Client wants to join game
    this->handler.RegisterHandler(
        std::function<void(MessagePtr, ShipzSession*)>(
            std::bind(&Server::HandleJoin, this, std::placeholders::_1,
                      std::placeholders::_2)),
        ConstructHeader(MessageType::REQUEST, JOIN_GAME));
}