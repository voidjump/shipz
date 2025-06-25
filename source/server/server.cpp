#include "server/server.h"

#include "messages/event.h"
#include "common/level.h"
#include "utils/log.h"
#include "common/other.h"
#include "messages/request.h"
#include "messages/response.h"
#include "net/session.h"
#include "messages/sync.h"
#include "utils/timer.h"

Server::Server(std::string level_name, const uint16_t listen_port,
               uint max_clients)
    : socket(io_context, listen_port) {
    this->level_name = level_name;
    this->max_clients = max_clients;
}

Server::~Server() {}

// Run the server
void Server::Run() {
    Init();
    if (!Load()) return;
    SpawnObjects();
    GameLoop();
}

// Spawn all default objects
void Server::SpawnObjects() {
    // Create base objects
    // Can't this be done by the level class?
    for( auto base_def : lvl.m_bases ) {
        auto b = new Base(base_def.owner, base_def.x, base_def.y);
    }
    GameState::Update();
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
    logger::info("loading level ", level_name);
    if (!lvl.Load()) {
        logger::error("failed to load level");
        return false;
    }
    return true;
}

// Update game state for clients periodically
void Server::WriteUpdates() {
    // Notify all players and send new player their information
    
    for (auto recipient : Player::instances) {
        // This is not very scalable
        // Emit all player states to recipient
        Player::EmitStates(recipient.second->session);
        // Team::EmitState(recipient.second->session);
        // Object::EmitStates(recipient.second->session);
    }
}

// Run the game loop; Start listening
void Server::GameLoop() {
    logger::info("starting server ", GetCurrentTime());
    
    //initialize callback
    socket.ReceiveUDP();

    Timer update(std::function<void()>(std::bind(&Server::WriteUpdates, this)),
                 10.0, true);
    Timer cleanup_stale_sessions(std::function<void()>(std::bind(&Server::PurgeStaleSessions, this)),
                 1.0, true);
    SDL_Event event;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                logger::info("quitting server ", GetCurrentTime());
                done = true;
            }
        }

        if (socket.Poll()) {
            auto recieved_packet = socket.GetPacket();
            auto session_id = recieved_packet->SessionID();
            // logger::debug("received a packet for session ", session_id);

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
                logger::debug("dropped packed due to invalid session");
            }
        }

        // Handle messages from clients
        HandleInboundMessages();

        // TODO: Game update functions
        // Updating object statuses, detecting collisions,
        // state machines etc.
        auto tick = Timer::Tick();
        Player::UpdateAll(tick);

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
        if (session->LastSendGreaterThan(80)) {
            auto packet = session->manager->CraftSendPacket();
            if (packet != nullptr) {
                this->socket.Send(*packet, session->endpoint);
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
    logger::debug("received unknown message: ", msg->AsDebugStr());
}

// A player wants to join
// TODO: Each client should have a unique client ID that is unrelated to the
// Session ID. This is the ID that players will carry between games and is
// Basically like their AccountID
void Server::HandleJoin(MessagePtr msg, ShipzSession* session) {
    if (session == nullptr) {
        // Not allowed without session
        return;
    }
    auto join = msg->As<RequestJoinGame>();
    logger::debug("client with name ", join->player_name, " requests join game");

    // Deny player if server full
    if (!(Player::instances.size() < this->max_clients)) {
        auto deny = std::make_shared<ResponseDenyJoin>("server full");
        session->manager->Write(deny);
        return;
    }

    // Allocate new player;
    auto new_player = new Player(session);

    logger::debug("Added new client id# ", new_player->player_id);
    new_player->name = join->player_name;

    session->client_id = new_player->player_id;

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
    logger::debug("client version ", (int)info->version, " requested information");

    session->Write<ResponseServerInformation>(
        SHIPZ_VERSION, Player::instances.size(), MAXPLAYERS, lvl.m_levelversion,
        lvl.m_filename);
    return;
}

// Create a session for address and port:
ShipzSession* Server::CreateSessionForClient(udp::endpoint client_endpoint) {
    // TODO: Refuse to create session for already existing endpoint
    if (ShipzSession::Exists(client_endpoint)) {
        logger::error(
            "Refusing to create new session; Addr and port already in use");
        return nullptr;
    }

    ShipzSession* new_session = new ShipzSession(client_endpoint);
    if (new_session == nullptr) {
        logger::error("Failed to create new session instance");
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
        logger::error("ignoring request to create session for existing session");
        return;
    }

    auto info = msg->As<SessionRequestSession>();
    logger::debug("client version ", (int)info->version, " requests new session");

    auto new_session =
        CreateSessionForClient(this->handler.CurrentOrigin());
    if (new_session == nullptr) {
        logger::error("Session request dropped");
        return;
    }
    SessionProvideSession provide_session_message(new_session->session_id);

    // Send Info; Note this has no session header
    Packet pack;
    pack.Append(provide_session_message);
    socket.Send(pack, handler.CurrentOrigin());
    return;
}

// Purge all stale sessions older than MAX_SESSION_AGE
void Server::PurgeStaleSessions() {
    // logger::debug("purging stale sessions..");
    uint64_t time = SDL_GetTicks();
    for (auto it = active_sessions.begin(); it != active_sessions.end();) {
        if (time - (*it)->last_active > MAX_SESSION_AGE) {
            logger::info("Dropping stale session ", (*it)->session_id);
            delete (*it);
            // Also remove any player with this session
            for (auto player : Player::instances) {
                if (player.second->session == *it) {
                    logger::debug("removing associated player ",
                               player.second->name);
                    delete player.second;
                    Player::instances.erase(player.first);
                    // TODO, send kick->event to all clients
                    // WriteAll(KickEvent(blabla))

                    break;
                }
            }
            it = active_sessions.erase(it);

        } else {
            ++it;
        }
    }
}

////////////////////// GAME ACTIONS /////////////////////

// Player requests to do an action
void Server::HandleAction(MessagePtr msg, ShipzSession* session) {
    if (session == nullptr) {
        // Not allowed without session
        return;
    }
    auto action = msg->As<RequestAction>();
    logger::debug("session ", session->session_id, " requests action");

    if (session->client_id == 0) {
        logger::error("client is not playing");
    } else {
        auto player = Player::GetByID(session->client_id);
        if (player == nullptr) {
            logger::error("client ID invalid, not an active player");
            return;
        }
        // Handle the event
        switch (action->action_id) {
        case PA_LIFTOFF:
            if(player->LiftOff()) {
                // Send Liftoff event to player
                for(auto recipient : active_sessions) {
                    recipient->Write<EventPlayerLiftOff>(player->player_id);
                }
            }
            break;

        case PA_SPAWN:
            if(player->Spawn()) {
                // Send Spawn event to all players
                auto base_id = Base::GetRandomRespawnBase(player->team);
                for(auto recipient : active_sessions) {
                    recipient->Write<EventPlayerSpawn>(player->player_id,
                                                         base_id);
                }
            }
        
            break;
        default:
            break;
    }
    }

    return;
}

void Server::HandleSyncWorld(MessagePtr msg, ShipzSession* session) {
    if (session == nullptr) {
        // Not allowed without session
        return;
    }
    for (auto base : Base::all_bases) {
        session->manager->Write(base->EmitSpawnMessage());
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

    // Client wants to join game
    this->handler.RegisterHandler(
        std::function<void(MessagePtr, ShipzSession*)>(
            std::bind(&Server::HandleAction, this, std::placeholders::_1,
                      std::placeholders::_2)),
        ConstructHeader(MessageType::REQUEST, REQUEST_ACTION));

    // Client wants to sync world
    this->handler.RegisterHandler(
        std::function<void(MessagePtr, ShipzSession*)>(
            std::bind(&Server::HandleSyncWorld, this, std::placeholders::_1,
                      std::placeholders::_2)),
        ConstructHeader(MessageType::REQUEST, SYNC_WORLD));
}