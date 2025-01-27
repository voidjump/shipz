#include "client.h"

#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <iostream>
#include <string>

#include "assets.h"
#include "bullet.h"
#include "base.h"
#include "event.h"
#include "font.h"
#include "gfx.h"
#include "level.h"
#include "log.h"
#include "net.h"
#include "other.h"
#include "packet.h"
#include "player.h"
#include "request.h"
#include "response.h"
#include "sound.h"
#include "team.h"
#include "types.h"
#include "sync.h"

// Construct a client
Client::Client(const char *server_hostname, 
               const char *player_name, 
               Uint16 listen_port, 
               Uint16 server_port)
    : socket(listen_port) {
    this->server_hostname = server_hostname;
    this->name = player_name;
    this->server_port = server_port;
}

// Run the game
void Client::Run() {
    this->Init();
    if(!this->Connect()) return;
    if(!this->Join()) return;
    if(!this->Load()) return;
    this->GameLoop();
}

// Initialize client
void Client::Init() {
    // Explicitly wrap std::bind with std::function
    this->handler.RegisterDefault(
        std::function<void(Message&)>(
            std::bind(&Client::HandleUnknownMessage, this, std::placeholders::_1)
        )
    );
    // Setup chat console
    this->console.SetHeight(3);
}

// Run client game loop
void Client::GameLoop() {
    done = false;
    this->SetupCallbacks();
    while (!done) {
        HandleInputs();

        Tick();

        if (socket.Poll()) {
            auto recieved_packet = socket.GetPacket();
            handler.HandlePacket(*recieved_packet);
        }

        UpdatePlayers();
        // TODO: Fix this
        // UpdateBullets(players);
        ClearOldExplosions();

        if ((SDL_GetTicks() - lastsendtime) > SEND_DELAY) {
            SendUpdate();
        }
        Draw();
    }
    this->Leave();
}

// Connect to the Server
bool Client::Connect() {
    Socket::ResolveHostname(this->server_hostname.c_str(), &this->server_address);
    int attempts = 0;

    // Create a request
    Packet pack;
    RequestGetServerInfo request(SHIPZ_VERSION);
    pack.Append(request);

    log::info("querying server status..");

    while (true) {
        socket.Send(pack, this->server_address, PORT_SERVER);
        SDL_Delay(500);

        if (socket.Poll()) {
            auto recieved_packet = socket.GetPacket();
            auto messages = recieved_packet->Read();
            for (Message &msg : messages) {
                if (msg.IsTypes(MessageType::RESPONSE, SERVER_INFO)) {
                    log::info("@ server responded...");
                    ResponseServerInformation *info = static_cast<ResponseServerInformation *>(&msg);

                    if (info->shipz_version != SHIPZ_VERSION) {
                        log::error("server is running shipz version:", info->shipz_version);
                        return false;
                    }
                    lvl.SetFile(info->level_filename);
                    return true;
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
bool Client::Join() {
    log::info("@ joining...");

    // Create a join request
    Packet pack;
    RequestJoinGame req(name);
    pack.Append(req);
 
    int attempts = 0;
    bool accepted = false;

    while(!accepted) {
        socket.Send(pack, server_address, server_port);
        if (socket.Poll()) {
            auto recieved_packet = socket.GetPacket();
            auto messages = recieved_packet->Read();
            for (Message &msg : messages) {
                if (msg.IsTypes(MessageType::RESPONSE, ACCEPT_JOIN)) {
                    log::info("@ join accepted...");
                    auto info = msg.As<ResponseAcceptJoin>();
                    self = new Player(info->client_id);
                    self->name = name;
                    self->self_sustaining = false;
                    this->players.push_back(self);
                    accepted = true;
                }
                if (msg.IsTypes(MessageType::RESPONSE, PLAYER_INFO)) {
                    log::info("@ player info");
                    auto info = msg.As<ResponsePlayerInformation>();
                    auto player = new Player(info->client_id);
                    player->team = info->team;
                    player->name = info->player_name;
                    player->self_sustaining = true;
                    this->players.push_back(self);
                }
            }
        }
        SDL_Delay(1000);
        attempts++;
        if(attempts > 5) {
            return false;
        }
    }
    return true;
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

            GetTyping(type_buffer, event.key.key, event.key.mod);

            if (event.key.key == SDLK_RETURN) {
                if (self->typing) {
                    this->SendChatLine();
                } else {
                    this->type_buffer.clear();
                    self->typing = true;
                }
            }
        }
    }

    keys = SDL_GetKeyboardState(NULL);
    if (self->status == PLAYER_STATUS::FLYING) {
        if (keys[SDL_SCANCODE_RIGHT]) {
            // rotate player clockwise
            self->Rotate(true);
        }
        if (keys[SDL_SCANCODE_LEFT]) {
            // rotate player counterclockwise
            self->Rotate(false);
        }
        if (keys[SDL_SCANCODE_UP]) {
            self->Thrust();
        }
        if (keys[SDL_SCANCODE_SPACE] && !self->typing) {
            if ((SDL_GetTicks() - self->lastliftofftime) > LIFTOFFSHOOTDELAY) {
                this->Shoot();
            }
        }
    }

    if (keys[SDL_SCANCODE_UP]) {
        if (self->status == PLAYER_STATUS::LANDED || self->status == PLAYER_STATUS::LANDEDBASE) {
            self->status = PLAYER_STATUS::LIFTOFF;
            self->y -= 10;
            self->lastliftofftime = SDL_GetTicks();
        }
        if (self->status == PLAYER_STATUS::LANDEDRESPAWN) {
            // no bullet delay after respawn, so don't reset lastliftofftime
            self->status = PLAYER_STATUS::LIFTOFF;
            self->y -= 10;
        }
    }
    if (self->status == PLAYER_STATUS::DEAD) {
        if (keys[SDL_SCANCODE_SPACE] && !self->typing) {
            // respawn
            self->status = PLAYER_STATUS::RESPAWN;
        }
    }

    if (!self->typing) {
        if (keys[SDL_SCANCODE_X]) {
            if (self->status != PLAYER_STATUS::DEAD && self->status != PLAYER_STATUS::RESPAWN) {
                self->status = PLAYER_STATUS::SUICIDE;
            }
        }
    }
}

// Update all players
void Client::UpdatePlayers() {
    for (auto &player : players) {
        player->Update();
    }
}

// Increase timer tick
void Client::Tick() {
    oldtime = newtime;
    newtime = float(SDL_GetTicks());
    deltatime = newtime - oldtime;
}

// Set all callbacks used during the game loop
void Client::SetupCallbacks() {
    this->handler.RegisterHandler(
        std::function<void(Message&)>(
            std::bind(&Client::HandleKicked, this, std::placeholders::_1)
        ),
        PLAYER_KICKED
    );
    this->handler.RegisterHandler(
        std::function<void(Message&)>(
            std::bind(&Client::HandlePlayerJoins, this, std::placeholders::_1)
        ),
        PLAYER_JOINS
    );
    this->handler.RegisterHandler(
        std::function<void(Message&)>(
            std::bind(&Client::HandlePlayerLeaves, this, std::placeholders::_1)
        ),
        PLAYER_LEAVES
    );
    this->handler.RegisterHandler(
        std::function<void(Message&)>(
            std::bind(&Client::HandleChat, this, std::placeholders::_1)
        ),
        CHAT_ALL
    );
    this->handler.RegisterHandler(
        std::function<void(Message&)>(
            std::bind(&Client::HandleObjectSpawn, this, std::placeholders::_1)
        ),
        OBJECT_SPAWN
    );
    this->handler.RegisterHandler(
        std::function<void(Message&)>(
            std::bind(&Client::HandleObjectUpdate, this, std::placeholders::_1)
        ),
        OBJECT_UPDATE
    );
    this->handler.RegisterHandler(
        std::function<void(Message&)>(
            std::bind(&Client::HandleObjectDestroy, this, std::placeholders::_1)
        ),
        OBJECT_DESTROY
    );
    this->handler.RegisterHandler(
        std::function<void(Message&)>(
            std::bind(&Client::HandlePlayerState, this, std::placeholders::_1)
        ),
        PLAYER_STATE
    );
    this->handler.RegisterHandler(
        std::function<void(Message&)>(
            std::bind(&Client::HandleTeamStates, this, std::placeholders::_1)
        ),
        TEAM_STATES
    );
}


// Send a state update to the server
// This contains both our position, state and the bullets we've fired
// TODO: We can add chat messages to this later
void Client::SendUpdate() {
    Packet pack;
    SyncPlayerState sync(self->client_id, self->status, self->typing, self->angle, self->x, self->y, self->vx, self->vy);
    pack.Append(sync);

    for ( auto &bullet : this->bullets_shot ) {
        pack.Append(bullet);
    }

    socket.Send(pack, this->server_address, PORT_SERVER);

    lastsendtime = SDL_GetTicks();
}

// Notify the server that we are leaving 
void Client::Leave() {
    // Create a request
    Packet pack;
    RequestLeaveGame request(this->client_id);
    pack.Append(request);
    socket.Send(pack, this->server_address, PORT_SERVER);
    // TODO: Send reliable, wait for ack with timeout
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

    for (auto player : players) {
        player->Draw();
    }

    DrawExplosions();

    if (self->status == PLAYER_STATUS::DEAD) {
        DrawFont(sansboldbig, "Press space to respawn.", XRES - 280, YRES - 13, FONT_COLOR::WHITE);
    }

    this->console.Draw();

    DrawIMG(scores, 0, YRES - 19);
    char tempstr[10];
    // draw blue frags:
    snprintf(tempstr, 10, "%i", blue_team.frags);
    DrawFont(sansbold, tempstr, 4, YRES - 17, FONT_COLOR::WHITE);
    // draw blue bases:
    snprintf(tempstr, 10, "%i", blue_team.bases);
    DrawFont(sansbold, tempstr, 29, YRES - 17, FONT_COLOR::WHITE);

    // draw red frags:
    snprintf(tempstr, 10, "%i", red_team.frags);
    DrawFont(sansbold, tempstr, 54, YRES - 17, FONT_COLOR::WHITE);

    // draw red bases:
    snprintf(tempstr, 10, "%i", red_team.bases);
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
        DrawFont(sansbold, type_buffer.c_str(), 5, YRES - 26, FONT_COLOR::WHITE);
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

    SDLNet_Quit();
}

// Spawn a bullet
void Client::Shoot() {
    switch (self->weapon ) {
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
    Packet pack;
    EventChat event(this->type_buffer.c_str(), this->client_id, 0);

    pack.Append(event);
    socket.Send(pack, this->server_address, PORT_SERVER);

    // Clear typing buffer
    type_buffer.clear();

    self->typing = false;
}

///////////////////////////////////////////////////////////////////////////////
// MESSAGE HANDLERS
///////////////////////////////////////////////////////////////////////////////

// Handle an unknown message
void Client::HandleUnknownMessage(Message &msg) {
    log::debug("received unknown message type");
    // TODO: Should we output the content of the message?
}

// Handle chat message
void Client::HandleChat(Message &msg) {
    auto event = msg.As<EventChat>();
    console.AddFromMessage(event);
}

// Handle a player join event 
void Client::HandlePlayerJoins(Message &msg) {
    auto event = msg.As<EventPlayerJoins>();
    AddPlayer(event->client_id, event->player_name, event->team);
    log::info("@ player ", event->player_name, " joined the server");
}

// Add a player to the game
void Client::AddPlayer(Uint16 id, std::string player_name, Uint8 team) {
    Player * new_player = new Player(id);
    new_player->Init();
    
    // Indicate that this updates coordinates by server
    new_player->self_sustaining = 1;

    new_player->team = team;
    new_player->name = player_name;

    this->players.push_back(new_player);
}

// Remove a player from the game
void Client::RemovePlayer(Uint16 id, std::string reason) {
    for (auto p = players.begin(); p != players.end();) {
        if( (*p)->client_id == id) {
            // Delete player object
            delete (*p);
            // Erase reference
            players.erase(p);
            log::info("@ player ", id, " left the server: ", reason);
            return;
        }
    }
}

void Client::HandlePlayerLeaves(Message& msg) {
    auto event = msg.As<EventPlayerLeaves>();
    RemovePlayer(event->client_id, event->leave_reason);
}

// Handle a message that a player was kicked
void Client::HandleKicked(Message &msg) {
    auto event = msg.As<EventPlayerKicked>();
    if(event->client_id == client_id) {
        log::info("@ kicked by server");
        done = true;
    } else {
        RemovePlayer(event->client_id, "Kicked by server");
    }
}

// An object is spawned
void Client::HandleObjectSpawn(Message& msg) {
    auto obj_spawn = msg.As<SyncObjectSpawn>();

    Object::HandleSpawn(obj_spawn);
}

// An object is updated
void Client::HandleObjectUpdate(Message& msg) {
    auto obj_update = msg.As<SyncObjectUpdate>();

    auto obj_instance = Object::GetByID(obj_update->id);
    obj_instance->HandleSync(obj_update);
}

// An object is destroyed
void Client::HandleObjectDestroy(Message& msg) {
    auto obj_destroy = msg.As<SyncObjectDestroy>();

    auto obj_instance = Object::GetByID(obj_destroy->id);
    obj_instance->HandleDestroy(obj_destroy);
}

// Update a player
void Client::HandlePlayerState(Message& msg) {
    auto player_state = msg.As<SyncPlayerState>();
    auto player = Player::GetByID(player_state->client_id);
    if( !player ) {
        return;
        log::error("recieved state for player that does not exist");
    }

    player->HandleUpdate(player_state);
}

// Update the team state, including the bases
void Client::HandleTeamStates(Message& msg) {
    auto sync = msg.As<SyncTeamStates>();

}