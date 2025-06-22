#include "client/client.h"

#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <iostream>
#include <string>

#include "common/assets.h"
#include "common/base.h"
#include "common/bullet.h"
#include "messages/event.h"
#include "client/font.h"
#include "client/gfx.h"
#include "common/level.h"
#include "utils/log.h"
#include "net/net.h"
#include "common/other.h"
#include "net/packet.h"
#include "common/player.h"
#include "messages/request.h"
#include "messages/response.h"
#include "net/session.h"
#include "client/sound.h"
#include "messages/sync.h"
#include "common/team.h"
#include "common/types.h"
#include "utils/timer.h"

// Construct a client
Client::Client(const char* server_hostname, const char* player_name,
               Uint16 listen_port, Uint16 server_port)
    : socket(io_context, listen_port) {
    this->server_hostname = server_hostname;
    this->name = player_name;
    this->server_port = server_port;
    this->listen_port = listen_port;
}

// Run the game
void Client::Run() {
    this->Init();
    // setup receive callback
    this->socket.ReceiveUDP(); 
    if ((this->session = this->Connect()) == nullptr) return;
    if (!this->JoinLoop()) return;
    if (!this->Load()) return;
    this->GameLoop();
}

// Initialize client
void Client::Init() {
    // Explicitly wrap std::bind with std::function
    this->handler.RegisterDefault(
        std::function<void(MessagePtr, ShipzSession*)>(
            std::bind(&Client::HandleUnknownMessage, this,
                      std::placeholders::_1, std::placeholders::_2)));
    // Setup chat console
    this->console.SetHeight(3);
}

void Client::Debug() {
    log::debug("x: ", self->x, " y: ", self->y);
    log::debug("vx: ", self->vx, " vy: ", self->vy);
    log::debug("fx: ", self->fx, " fy: ", self->fy);
}

// Run client game loop
void Client::GameLoop() {
    done = false;
    Timer t_fps(std::function<void()>(std::bind(&Client::Draw, this)),
                 60.0, true);
    Timer t_update(std::function<void()>(std::bind(&Client::SendUpdate, this)),
                 10.0, true);
    Timer t_debug(std::function<void()>(std::bind(&Client::Debug, this)),
                 0.5, true);
    this->handler.Clear();
    this->SetupCallbacks();
    session->Write<RequestSyncWorld>(self->team);
    while (!done) {
        HandleInputs();

        if (socket.Poll()) {
            auto recieved_packet = socket.GetPacket();
            if(recieved_packet->SessionID() != session->session_id) {
                log::error("received packet with wrong session ID");
            } else {
                session->manager->HandleReceivedPacket(*recieved_packet);
                handler.HandleMessageList(session->manager->Read(), session);
            }
        }

        auto tick_time = Timer::Tick();
        Player::UpdateAll(tick_time);
        ClearOldExplosions();

    }
    this->Leave();
}

// Connect to the Server
ShipzSession* Client::Connect() {
    this->server_endpoint = Socket::ResolveHostname(this->io_context, this->server_hostname.c_str(), PORT_SERVER);
    int attempts = 0;

    // Create a request
    Packet pack;
    SessionRequestSession request(SHIPZ_VERSION, this->listen_port);
    pack.Append(request);

    log::info("querying server status..");

    while (true) {
        socket.Send(pack, this->server_endpoint);
        // Wait a bit for a reply to not spam the server with session requests
        SDL_Delay(300);

        if (socket.Poll()) {
            auto recieved_packet = socket.GetPacket();
            auto messages = recieved_packet->Read();
            for (auto msg : messages) {
                if (msg->IsTypes(MessageType::SESSION, PROVIDE_SESSION)) {
                    log::info("obtained session...");
                    SessionProvideSession* session_msg =
                        msg->As<SessionProvideSession>();
                    session_msg->LogDebug();

                    // Create new session from provide session message
                    return new ShipzSession(this->server_endpoint,
                                            session_msg->session_id);
                }
            }
        }
        attempts++;
        log::info(".");
        if (attempts == 10) {
            log::error("didn't receive response from server.");
            exit(1);
        }
    }
}

// Join a server
bool Client::JoinLoop() {
    state = S_INITIAL;
    auto time = SDL_GetTicks();
    int attempts = 1;

    this->handler.Clear();
    SetupJoinCallsbacks();

    while (!(state == S_ACCEPTED || state == S_SERVER_FULL || state == S_TIMEOUT ||
            state == S_ERROR || state == S_DENIED || state == S_VERSION_MISMATCH)) {
        switch(state) {
            case S_INITIAL:
                log::debug("Requesting server information");
                // We request information from the server
                session->Write<RequestGetServerInfo>(SHIPZ_VERSION);
                time = SDL_GetTicks();
                state = S_AWAITING_INFO;
                break;
            case S_RECEIVED_INFO:
                attempts = 0;
                time = SDL_GetTicks();
                state = S_JOINING_SERVER;
                break;
            case S_JOINING_SERVER:
                // Request to join the game
                log::debug("Requesting to join game");
                session->Write<RequestJoinGame>(name);
                time = SDL_GetTicks();
                state = S_AWAITING_JOIN;
                break;
            // retry after 500 ms
            case S_AWAITING_INFO:
                if ((SDL_GetTicks() - time) > 500) {
                    log::debug("No response for info, retrying");
                    attempts++;
                    state = (attempts > 5) ? S_TIMEOUT : S_INITIAL;
                }
                break;
            case S_AWAITING_JOIN:
                if ((SDL_GetTicks() - time) > 500) {
                    log::debug("No response for join, retrying");
                    attempts++;
                    state = (attempts > 5) ? S_TIMEOUT : S_JOINING_SERVER;
                }
                break;
            default:
                break;
        }

        // Send any outbound messages
        if( session->LastSendGreaterThan(80)) {
            auto packet = session->manager->CraftSendPacket();
            if (packet != nullptr) {
                this->socket.Send(*packet, session->endpoint);
                session->SendTick();
            }
        }   

        // Check socket
        if (socket.Poll()) {
            auto recieved_packet = socket.GetPacket();
            session->manager->HandleReceivedPacket(*recieved_packet);
        }
        handler.HandleMessageList(session->manager->Read(), session);
    }
    return state == S_ACCEPTED;
}

void Client::HandleInputs() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            done = true;
        }
        if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.key == SDLK_ESCAPE) {
                done = true;
            }

            if (event.key.key == SDLK_TAB && !self->typing) {
                switch (self->weapon) {
                    case WEAPON_BULLET:
                        self->weapon = WEAPON_ROCKET;
                        break;
                    case WEAPON_ROCKET:
                        self->weapon = WEAPON_MINE;
                        break;
                    case WEAPON_MINE:
                        self->weapon = WEAPON_BULLET;
                        break;
                }
                PlaySound(weaponswitch);
            }

            if(self->typing) {
                GetTyping(type_buffer, event.key.key, event.key.mod);
            }

            if (event.key.key == SDLK_RETURN) {
                if (self->typing) {
                    this->SendChatLine();
                } else {
                    this->type_buffer.clear();
                    self->typing = true;
                }
            }

            if (event.key.key == SDLK_UP) {
                if (self->IsLanded()) {
                    SendAction(PA_LIFTOFF);
                }
            }
        
            if (event.key.key == SDLK_SPACE) {
                if (!self->typing && !self->IsAlive()) {
                    log::info("trying to spawn");
                    SendAction(PA_SPAWN);
                }
            }
        }
    }

    keys = SDL_GetKeyboardState(NULL);
    if (self->status == PLAYER_STATUS::FLYING) {
        if (keys[SDL_SCANCODE_RIGHT]) {
            // rotate player clockwise
            self->Rotate(true, Timer::LastTick());
        }
        if (keys[SDL_SCANCODE_LEFT]) {
            // rotate player counterclockwise
            self->Rotate(false, Timer::LastTick());
        }
        if (keys[SDL_SCANCODE_UP]) {
            self->engine_on = true;
        }
        if (keys[SDL_SCANCODE_SPACE] && !self->typing) {
            if ((SDL_GetTicks() - self->lastliftofftime) > LIFTOFFSHOOTDELAY) {
                this->Shoot();
            }
        }
    }


}

// Request to do an action from the server
// These are influential state changes such as respawning, lifting off, etc.
void Client::SendAction(uint16_t action) {
    session->Write<RequestAction>(action);
}

// Set up all callbacks used during join loop
void Client::SetupJoinCallsbacks() {
    this->handler.RegisterHandler(
        std::function<void(MessagePtr, ShipzSession*)>(
            std::bind(&Client::HandleServerInfo, this, std::placeholders::_1,
                      std::placeholders::_2)),
    ConstructHeader(MessageType::RESPONSE, SERVER_INFO));
    this->handler.RegisterHandler(
        std::function<void(MessagePtr, ShipzSession*)>(
            std::bind(&Client::HandleAcceptJoin, this, std::placeholders::_1,
                      std::placeholders::_2)),
    ConstructHeader(MessageType::RESPONSE, ACCEPT_JOIN));
    this->handler.RegisterHandler(
        std::function<void(MessagePtr, ShipzSession*)>(
            std::bind(&Client::HandleDenyJoin, this, std::placeholders::_1,
                      std::placeholders::_2)),
    ConstructHeader(MessageType::RESPONSE, DENY_JOIN));
    this->handler.RegisterHandler(
        std::function<void(MessagePtr, ShipzSession*)>(
            std::bind(&Client::HandlePlayerInfo, this, std::placeholders::_1,
                      std::placeholders::_2)),
    ConstructHeader(MessageType::RESPONSE, PLAYER_INFO));
}

// Set all callbacks used during the game loop
void Client::SetupCallbacks() {
    this->handler.RegisterHandler(
        std::function<void(MessagePtr, ShipzSession*)>(
            std::bind(&Client::HandlePlayerJoins, this, std::placeholders::_1,
                      std::placeholders::_2)),
    ConstructHeader(MessageType::EVENT, PLAYER_JOINS));
    this->handler.RegisterHandler(
        std::function<void(MessagePtr, ShipzSession*)>(
            std::bind(&Client::HandlePlayerLeaves, this, std::placeholders::_1,
                      std::placeholders::_2)),
    ConstructHeader(MessageType::EVENT, PLAYER_LEAVES));
    this->handler.RegisterHandler(
        std::function<void(MessagePtr, ShipzSession*)>(
            std::bind(&Client::HandlePlayerInfo, this, std::placeholders::_1,
                      std::placeholders::_2)),
    ConstructHeader(MessageType::RESPONSE, PLAYER_INFO));
    this->handler.RegisterHandler(
        std::function<void(MessagePtr, ShipzSession*)>(
            std::bind(&Client::HandleChat, this, std::placeholders::_1,
                      std::placeholders::_2)),
    ConstructHeader(MessageType::EVENT, CHAT_ALL));
    this->handler.RegisterHandler(
        std::function<void(MessagePtr, ShipzSession*)>(
            std::bind(&Client::HandleObjectSpawn, this, std::placeholders::_1,
                      std::placeholders::_2)),
    ConstructHeader(MessageType::EVENT, OBJECT_SPAWN));
    this->handler.RegisterHandler(
        std::function<void(MessagePtr, ShipzSession*)>(
            std::bind(&Client::HandleObjectUpdate, this, std::placeholders::_1,
                      std::placeholders::_2)),
    ConstructHeader(MessageType::SYNC, OBJECT_UPDATE));
    this->handler.RegisterHandler(
        std::function<void(MessagePtr, ShipzSession*)>(
            std::bind(&Client::HandleObjectDestroy, this, std::placeholders::_1,
                      std::placeholders::_2)),
    ConstructHeader(MessageType::EVENT, OBJECT_DESTROY));
    this->handler.RegisterHandler(
        std::function<void(MessagePtr, ShipzSession*)>(
            std::bind(&Client::HandlePlayerState, this, std::placeholders::_1,
                      std::placeholders::_2)),
    ConstructHeader(MessageType::SYNC, PLAYER_STATE));
    this->handler.RegisterHandler(
        std::function<void(MessagePtr, ShipzSession*)>(
            std::bind(&Client::HandleTeamStates, this, std::placeholders::_1,
                      std::placeholders::_2)),
    ConstructHeader(MessageType::SYNC, TEAM_STATES));
    this->handler.RegisterHandler(
        std::function<void(MessagePtr, ShipzSession*)>(
            std::bind(&Client::HandleSpawnEvent, this, std::placeholders::_1,
                      std::placeholders::_2)),
    ConstructHeader(MessageType::EVENT, PLAYER_SPAWN));
    this->handler.RegisterHandler(
        std::function<void(MessagePtr, ShipzSession*)>(
            std::bind(&Client::HandleLiftOffEvent, this, std::placeholders::_1,
                      std::placeholders::_2)),
    ConstructHeader(MessageType::EVENT, PLAYER_LIFTOFF));
}

// Send a state update to the server
// This contains both our position, state and the bullets we've fired
// TODO: We can add chat messages to this later
void Client::SendUpdate() {
    session->Write<SyncPlayerState>(self->player_id, self->status, self->typing,
                         self->angle, self->x, self->y, self->vx, self->vy);
    auto packet = session->manager->CraftSendPacket();
    if (packet != nullptr) {
        this->socket.Send(*packet, session->endpoint);
    }
}

// Notify the server that we are leaving
void Client::Leave() {
    session->Write<RequestLeaveGame>(this->client_id);
    // TODO: Wait for ack
    done = true;
}

bool Client::Load() {
    if (!lvl.Load()) {
        log::error("failed to load level");
        return false;
    }

    InitSound();
    InitFont();
    LoadAssets();
    CreateGonLookup();

    CleanAllExplosions();
    return true;
}

// Perform UI/Game drawing
// TODO: Renderlayers
// TODO: HW accel SDL3
void Client::Draw() {
    AdjustViewport(this->self);  // focus viewport on self
                                 // maybe in later stage we could use this for
                                 // spectator mode.. so that when player is dead
                                 // he can view another (friendly?) player.

    Slock(screen);
    DrawIMG(level, 0, 0, XRES, YRES, viewportx, viewporty);

    Object::DrawAll();

    for (auto player : Player::instances) {
        player.second->Draw();
    }

    DrawExplosions();

    if (self->status == PLAYER_STATUS::DEAD) {
        DrawFont(sansboldbig, "Press space to respawn.", XRES - 280, YRES - 13,
                 FONT_COLOR::WHITE);
    }

    this->console.Draw();

    DrawIMG(scores, 0, YRES - 19);
    char tempstr[10];
    // draw blue bases:
    snprintf(tempstr, 10, "%i", GameState::blue_bases);
    DrawFont(sansbold, tempstr, 29, YRES - 17, FONT_COLOR::WHITE);

    // draw red bases:
    snprintf(tempstr, 10, "%i", GameState::red_bases);
    DrawFont(sansbold, tempstr, 79, YRES - 17, FONT_COLOR::WHITE);

    switch (self->weapon) {
        case WEAPON_BULLET:
            DrawIMG(bullet_icon, 110, YRES - 18);
            break;
        case WEAPON_ROCKET:
            DrawIMG(rocket_icon, 110, YRES - 18);
            break;
        case WEAPON_MINE:
            DrawIMG(mine_icon, 110, YRES - 18);
            break;
    }

    if (self->typing) {
        DrawFont(sansbold, type_buffer.c_str(), 5, YRES - 26,
                 FONT_COLOR::WHITE);
    }

    UpdateScreen();
    Sulock(screen);
}

Client::~Client() {
    SDL_DestroySurface(shipred);
    SDL_DestroySurface(shipblue);
    SDL_DestroySurface(chatpixmap);
    SDL_DestroySurface(level);
    SDL_DestroySurface(crosshairred);
    SDL_DestroySurface(crosshairblue);
    SDL_DestroySurface(bulletpixmap);
    SDL_DestroySurface(rocketpixmap);
    SDL_DestroySurface(minepixmap);
    SDL_DestroySurface(basesimg);
    SDL_DestroySurface(rocket_icon);
    SDL_DestroySurface(bullet_icon);
    SDL_DestroySurface(mine_icon);
    SDL_DestroySurface(scores);

    Mix_FreeChunk(explodesound);
    Mix_FreeChunk(rocketsound);
    Mix_FreeChunk(weaponswitch);
    Mix_CloseAudio();

    TTF_CloseFont(sansbold);
    TTF_CloseFont(sansboldbig);

    TTF_Quit();
}

// Spawn a bullet
void Client::Shoot() {
    switch (self->weapon) {
        case WEAPON_BULLET:
            if ((SDL_GetTicks() - self->lastshottime) <= BULLETDELAY) return;
            this->bullets_shot.push_back(Bullet::Shoot(self));
            break;
        case WEAPON_ROCKET:
            if ((SDL_GetTicks() - self->lastshottime) <= ROCKETDELAY) return;
            this->bullets_shot.push_back(Rocket::Shoot(self));
            break;
        case WEAPON_MINE:
            if ((SDL_GetTicks() - self->lastshottime) <= MINEDELAY) return;
            this->bullets_shot.push_back(Mine::Shoot(self));
            break;
        default:
            return;
    }
    self->lastshottime = SDL_GetTicks();
}

// This is called when a player presses return
// It transmits the current chat entered in the chat buffer to the server.
void Client::SendChatLine() {
    session->Write<EventChat>(this->type_buffer.c_str(), this->client_id, 0);
    // Clear typing buffer
    type_buffer.clear();
    self->typing = false;
}

///////////////////////////////////////////////////////////////////////////////
// MESSAGE HANDLERS
///////////////////////////////////////////////////////////////////////////////

// Handle an unknown message
void Client::HandleUnknownMessage(MessagePtr msg, ShipzSession* session) {
    log::debug("received unknown message type: ", msg->AsDebugStr());
}

// Handle chat message
void Client::HandleChat(MessagePtr msg, ShipzSession* session) {
    auto event = msg->As<EventChat>();
    console.AddFromMessage(event);
}

// Handle a player join event
void Client::HandlePlayerJoins(MessagePtr msg, ShipzSession* session) {
    auto event = msg->As<EventPlayerJoins>();
    // TODO: Implement
    AddPlayer(event->client_id, event->player_name, event->team);
    log::info("player ", event->player_name, " joined the server");
}

// Add a player to the game
void Client::AddPlayer(Uint16 id, std::string player_name, Uint8 team) {
    Player* new_player = new Player(id);
    new_player->Init();

    // Indicate that this updates coordinates by server
    new_player->is_local = true;

    new_player->team = team;
    new_player->name = player_name;
}

// Remove a player from the game
void Client::RemovePlayer(Uint16 id, std::string reason) {
    Player::Remove(id);
    log::info("player ", id, " left the server: ", reason);
}

void Client::HandlePlayerLeaves(MessagePtr msg, ShipzSession* session) {
    auto event = msg->As<EventPlayerLeaves>();
    RemovePlayer(event->client_id, event->leave_reason);
}

// An object is spawned
void Client::HandleObjectSpawn(MessagePtr msg, ShipzSession* session) {
    auto obj_spawn = msg->As<EventObjectSpawn>();
    log::debug("called for obj with id ", obj_spawn->id);

    Object::HandleSpawn(obj_spawn);
    if(obj_spawn->type == OBJECT_TYPE::BASE) {
        GameState::Update();
    }
}

// An object is updated
void Client::HandleObjectUpdate(MessagePtr msg, ShipzSession* session) {
    auto obj_update = msg->As<SyncObjectUpdate>();

    auto obj_instance = Object::GetByID(obj_update->id);
    obj_instance->HandleSync(obj_update);
}

// An object is destroyed
void Client::HandleObjectDestroy(MessagePtr msg, ShipzSession* session) {
    auto obj_destroy = msg->As<EventObjectDestroy>();

    auto obj_instance = Object::GetByID(obj_destroy->id);
    obj_instance->HandleDestroy(obj_destroy);
}

// Update a player
void Client::HandlePlayerState(MessagePtr msg, ShipzSession* session) {
    auto player_state = msg->As<SyncPlayerState>();
    auto player = Player::GetByID(player_state->client_id);
    if (!player) {
        return;
        log::error("recieved state for player that does not exist");
    }

    player->HandleUpdate(player_state);
}

// Update the team state, including the bases
void Client::HandleTeamStates(MessagePtr msg, ShipzSession* session) {
    auto sync = msg->As<SyncTeamStates>();
}

void Client::HandleServerInfo(MessagePtr msg, ShipzSession* session) {
    auto info = msg->As<ResponseServerInformation>();
    log::info("server responded...");
    info->LogDebug();

    if (info->shipz_version != SHIPZ_VERSION) {
        log::error("server is running shipz version:",
                    info->shipz_version);
        state = S_ERROR;
        return;
    }
    if (info->max_players == info->number_of_players) {
        log::info("server is full!");
        state = S_SERVER_FULL;
        return;

    }
    lvl.SetFile(info->level_filename);
    state = S_RECEIVED_INFO;
}

void Client::HandleAcceptJoin(MessagePtr msg, ShipzSession* session) {
    auto accept = msg->As<ResponseAcceptJoin>();
    log::info("join accepted...");
    accept->LogDebug();
    self = new Player(accept->client_id);
    self->name = name;
    self->is_local = false;
    state = S_ACCEPTED; 
}

void Client::HandleDenyJoin(MessagePtr msg, ShipzSession* session) {
    auto deny = msg->As<ResponseDenyJoin>();
    log::info("Server denied join, reason: ", deny->reason);
    state = S_DENIED;
}

void Client::HandlePlayerInfo(MessagePtr msg, ShipzSession* session) {
    log::info("player info");
    auto info = msg->As<ResponsePlayerInformation>();
    info->LogDebug();
    AddPlayer(info->client_id, info->player_name, info->team);
}

void Client::HandleSpawnEvent(MessagePtr msg, ShipzSession* session) {
    auto spawn = msg->As<EventPlayerSpawn>();
    log::info("spawning at base ", spawn->base_id);
    // TODO: IMPLEMENT FOR OTHER PLAYERS !!!!!!
    // TODO: IMPLEMENT FOR OTHER PLAYERS !!!!!!
    // TODO: IMPLEMENT FOR OTHER PLAYERS !!!!!!
    // TODO: IMPLEMENT FOR OTHER PLAYERS !!!!!!
    // TODO: IMPLEMENT FOR OTHER PLAYERS !!!!!!
    // Update coordinates to base
    
    auto obj = Object::GetByID(spawn->base_id);
    auto base = std::dynamic_pointer_cast<Base>(obj);
    self->x = base->x;
    self->y = base->y - BASE_SPAWN_Y_DELTA;
}

void Client::HandleLiftOffEvent(MessagePtr msg, ShipzSession* session) {
    log::info("liftoff event");
    auto liftoff = msg->As<EventPlayerLiftOff>();
    self->status = PLAYER_STATUS::FLYING;
}