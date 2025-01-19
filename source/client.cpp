#include "client.h"

#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <iostream>
#include <string>

#include "assets.h"
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

// TODO: Candidates for bitwise state flags

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
    Socket::ResolveHostname(this->server_hostname.c_str(), this->server_address);
    int attempts = 0;

    // Create a request
    Packet pack;
    RequestGetServerInfo request(name, SHIPZ_VERSION);
    pack.Append(request);

    log::info("@ querying server status..");

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
        }
    }
}

// Join a server
bool Client::Join() {
    log::info("@ joining...");

    // Create a join request
    Packet pack;
    RequestJoinGame req(std::string(name));
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
                    self = new Player();
                    // TODO: Modify constructor of player to take msg?
                    self->client_id = info->client_id;
                    self->name = name;
                    self->self_sustaining = false;
                    this->players.push_back(self);
                    accepted = true;
                }
                if (msg.IsTypes(MessageType::RESPONSE, PLAYER_INFO)) {
                    log::info("@ player info");
                    auto info = msg.As<ResponsePlayerInformation>();
                    auto player = new Player();
                    player->client_id = info->client_id;
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

            GetTyping(current_typing, event.key.key, event.key.mod);

            if (event.key.key == SDLK_RETURN) {
                if (self->typing) {
                    this->SendChatLine();
                } else {
                    this->current_typing.clear();
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
                if (self->weapon == WEAPON_BULLET) {
                    if ((SDL_GetTicks() - self->lastshottime) > BULLETDELAY) {
                        self->bulletshotnr = ShootBullet(self, client_id);
                        if (self->bulletshotnr != 6666) {
                            self->lastshottime = SDL_GetTicks();
                            self->bullet_shot = 1;
                        } else {
                            self->bulletshotnr = 0;
                        }
                    }
                }
                if (self->weapon == WEAPON_ROCKET) {
                    if ((SDL_GetTicks() - self->lastshottime) > ROCKETDELAY) {
                        self->bulletshotnr = ShootBullet(self, client_id);
                        if (self->bulletshotnr != 6666) {
                            self->lastshottime = SDL_GetTicks();
                            self->bullet_shot = 1;
                        } else {
                            self->bulletshotnr = 0;
                        }
                    }
                }
                if (self->weapon == WEAPON_MINE) {
                    if ((SDL_GetTicks() - self->lastshottime) > MINEDELAY) {
                        self->bulletshotnr = ShootBullet(self, client_id);
                        if (self->bulletshotnr != 6666) {
                            self->lastshottime = SDL_GetTicks();
                            self->bullet_shot = 1;
                        } else {
                            self->bulletshotnr = 0;
                        }
                    }
                }
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

void Client::UpdatePlayers() {
    for (int up = 0; up < 8; up++) {
        if (players[up].playing) {
            if (players[up].status == PLAYER_STATUS::FLYING) {
                players[up].Update();
            }
        }
    }
}

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


// should send the following stuff:
// 040 PLAYER STATUS ANGLE X Y VX VY WEAPON NUMBULS ( BULX BULY BULVX BULVY *
// NUMBULS )
void Client::SendUpdate() {
    this->send_buffer.Clear();

    this->send_buffer.Write8(SHIPZ_MESSAGE::UPDATE);
    this->send_buffer.Write8(my_player_nr);
    this->send_buffer.Write8(self->status);

    this->send_buffer.Write16(Sint16(self->shipframe));
    this->send_buffer.Write16(Sint16(self->typing));
    this->send_buffer.Write16(Sint16(self->x));
    this->send_buffer.Write16(Sint16(self->y));
    this->send_buffer.Write16(Sint16(self->vx));
    this->send_buffer.Write16(Sint16(self->vy));

    if (self->bullet_shot) {
        this->send_buffer.Write16(Sint16(self->bulletshotnr));
        this->send_buffer.Write16(Sint16(bullets[self->bulletshotnr].type));
        if (bullets[self->bulletshotnr].type == WEAPON_BULLET || bullets[self->bulletshotnr].type == WEAPON_MINE) {
            this->send_buffer.Write16(Sint16(bullets[self->bulletshotnr].x));
            this->send_buffer.Write16(Sint16(bullets[self->bulletshotnr].y));
            this->send_buffer.Write16(Sint16(bullets[self->bulletshotnr].vx));
            this->send_buffer.Write16(Sint16(bullets[self->bulletshotnr].vy));
        }
        if (bullets[self->bulletshotnr].type == WEAPON_ROCKET) {
            // Why is this here?
            this->send_buffer.Write16(0);
            this->send_buffer.Write16(Sint16(bullets[self->bulletshotnr].angle));
            this->send_buffer.Write16(Sint16(bullets[self->bulletshotnr].x));
            this->send_buffer.Write16(Sint16(bullets[self->bulletshotnr].y));
        }
        self->bullet_shot = false;
        self->bulletshotnr = 0;
    } else {
        this->send_buffer.WriteBytes(12, 0x00);
    }

    this->SendBuffer();

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
        log::error("failed to load level!");
        return false;
    }

    // TODO: move this to the right places
    InitSound();
    InitFont();
    LoadAssets();
    CreateGonLookup();

    for (int zb = 0; zb < NUMBEROFBULLETS; zb++) {
        CleanBullet(zb);
    }

    CleanAllExplosions();
}

// Perform UI/Game drawing
void Client::Draw() {
    AdjustViewport(this->self);  // focus viewport on self
                                 // maybe in later stage we could use this for
                                 // spectator mode.. so that when player is dead
                                 // he can view another (friendly?) player.

    Slock(screen);
    DrawIMG(level, 0, 0, XRES, YRES, viewportx, viewporty);

    DrawBullets(bulletpixmap);

    for (int up = 0; up < 8; up++) {
        // if ship = 1... etc.. do later
        if (players[up].playing && players[up].status != PLAYER_STATUS::DEAD &&
            players[up].status != PLAYER_STATUS::RESPAWN) {
            if (players[up].Team == SHIPZ_TEAM::RED) {
                DrawPlayer(shipred, &players[up]);
            }
            if (players[up].Team == SHIPZ_TEAM::BLUE) {
                DrawPlayer(shipblue, &players[up]);
            }
        }
    }
    DrawBases(basesimg);
    if (self->status == PLAYER_STATUS::DEAD) {
        DrawFont(sansboldbig, "Press space to respawn.", XRES - 280, YRES - 13, FONT_COLOR::WHITE);
    }

    DrawExplosions();

    if (strlen(chat1)) {
        DrawFont(sansbold, chat1, 5, 5, FONT_COLOR::WHITE);
    }
    if (strlen(chat2)) {
        DrawFont(sansbold, chat2, 5, 16, FONT_COLOR::WHITE);
    }
    if (strlen(chat3)) {
        DrawFont(sansbold, chat3, 5, 27, FONT_COLOR::WHITE);
    }

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
        DrawFont(sansbold, type_buffer.AsString(), 5, YRES - 26, FONT_COLOR::WHITE);
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

// void Client::TakeScreenShot() {
//     char tempstr[30];
//     memset(tempstr, '\0', sizeof(tempstr));
//     snprintf(tempstr, 30, "ss%d.bmp", screenshotcounter);
//     SDL_SaveBMP(screen, tempstr);
//     this->screenshotcounter++;
// }

// This is called when a player presses return
// It transmits the current chat entered in the chat buffer to the server.
void Client::SendChatLine() {
    Packet pack;
    EventChat event(this->current_typing.str(), this->client_id, 0);

    pack.Append(event);
    socket.Send(pack, this->server_address, PORT_SERVER);

    // Clear typing buffer
    current_typing.clear();

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
    AddPlayer(event->client_id, event->player_name);
    log::info("@ player ", event->player_name, " joined the server");
}

// Add a player to the game
void Client::AddPlayer(Uint16 id, std::string player_name, Uint8 team) {
    Player * new_player = new Player();
    new_player->Init();
    
    // Indicate that this updates coordinates by server
    new_player->self_sustaining = 1;

    new_player->team = team;
    new_player->name = player_name;
    new_player->client_id = id;

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

}

// An object is updated
void Client::HandleObjectUpdate(Message& msg) {

    Sint16 bullet_count = 0;
    bullet_count = (Sint16)receive_buffer.Read16("bullet_count");
    if (bullet_count != 0) {
        std::cout << "bullet update count: " << bullet_count << std::endl;
        for (int gcb = 0; gcb < bullet_count; gcb++) {
            Sint16 num = (Sint16)receive_buffer.Read16("number");
            if (bullets[num].type == WEAPON_MINE || bullets[num].type == WEAPON_ROCKET) {
                std::cout << "New explosion for bullet " << num << std::endl;
                NewExplosion(int(bullets[num].x), int(bullets[num].y));
            }
            CleanBullet(int(num));
        }
    }
}
// An object is destroyed
void Client::HandleObjectDestroy(Message& msg) {

}
// Update a player
void Client::HandlePlayerState(Message& msg) {
    // FOR OTHER PLAYERS:
            int tempstat = (Sint16)receive_buffer.Read16("status");

            players[rp].shipframe = (Sint16)receive_buffer.Read16("frame");
            players[rp].typing = (Sint16)receive_buffer.Read16("typing");
            players[rp].x = (Sint16)receive_buffer.Read16("x");
            players[rp].y = (Sint16)receive_buffer.Read16("y");
            players[rp].vx = (Sint16)receive_buffer.Read16("vx");
            players[rp].vy = (Sint16)receive_buffer.Read16("vy");

            if (tempstat == PLAYER_STATUS::FLYING &&
                (players[rp].status == PLAYER_STATUS::LANDED || players[rp].status == PLAYER_STATUS::LANDEDBASE)) {
                players[rp].lastliftofftime = SDL_GetTicks();
            }
            if (tempstat == PLAYER_STATUS::DEAD && players[rp].status != PLAYER_STATUS::DEAD) {
                NewExplosion(int(players[rp].x), int(players[rp].y));
            }
            players[rp].status = tempstat;

            Uint8 bullet_shot = (bool)receive_buffer.Read8("bullet_shot");
            if (!bullet_shot) {
                continue;
            }

            Sint16 tx, ty, tvx, tvy, tn, tbultyp;
            tn = (Sint16)receive_buffer.Read16("number");
            tbultyp = (Sint16)receive_buffer.Read16("type");
            tx = (Sint16)receive_buffer.Read16("tx");
            ty = (Sint16)receive_buffer.Read16("ty");
            tvx = (Sint16)receive_buffer.Read16("tvx");
            tvy = (Sint16)receive_buffer.Read16("tvy");
            if (tx == 0 && ty == 0 && tvx == 0 && tvy == 0 && tn == 0 && tbultyp == 0) {
                // this is an empty Bullet
            } else {
                if (tbultyp == WEAPON_MINE) {
                    bullets[tn].x = (float)tx;
                    bullets[tn].y = (float)ty;
                    bullets[tn].minelaidtime = SDL_GetTicks();
                }
                if (tbultyp == WEAPON_BULLET) {
                    bullets[tn].x = (float)tx;
                    bullets[tn].y = (float)ty;
                    bullets[tn].vx = (float)tvx;
                    bullets[tn].vy = (float)tvy;
                }
                if (tbultyp == WEAPON_ROCKET) {
                    bullets[tn].x = (float)tvx;
                    bullets[tn].y = (float)tvy;
                    bullets[tn].angle = (float)ty;
                    PlaySound(rocketsound);
                }
                bullets[tn].type = tbultyp;
                bullets[tn].active = true;
                bullets[tn].collide = false;
                bullets[tn].owner = rp + 1;
            }
        
 // FOR SELF: (THIS CLIENT)
            int tempstatus = (Sint16)receive_buffer.Read16("status");
            receive_buffer.Read16("(frame)");
            receive_buffer.Read16("(typing)");

            Sint16 tx, ty;
            tx = (Sint16)receive_buffer.Read16("x");
            ty = (Sint16)receive_buffer.Read16("y");
            receive_buffer.Read16("(vx)");
            receive_buffer.Read16("(vy)");
            receive_buffer.Read8("(bullet_shot)");

            if (tempstatus == PLAYER_STATUS::DEAD && self->status == PLAYER_STATUS::SUICIDE) {
                std::cout << "we have just suicided!" << std::endl;
                NewExplosion(int(self->x), int(self->y));
                self->status = PLAYER_STATUS::DEAD;
            }
            if (tempstatus == PLAYER_STATUS::JUSTCOLLIDEDROCK && self->status == PLAYER_STATUS::FLYING) {
                std::cout << "we just collided with a rock!" << std::endl;
                NewExplosion(int(self->x), int(self->y));
                self->status = PLAYER_STATUS::DEAD;
            }
            if (tempstatus == PLAYER_STATUS::JUSTCOLLIDEDBASE && self->status == PLAYER_STATUS::FLYING) {
                std::cout << "we just collided with Base!" << std::endl;
                NewExplosion(int(self->x), int(self->y));
                self->status = PLAYER_STATUS::DEAD;
            }
            if (tempstatus == PLAYER_STATUS::JUSTSHOT && self->status == PLAYER_STATUS::FLYING) {
                std::cout << "we were just shot!" << std::endl;
                NewExplosion(int(self->x), int(self->y));
                self->status = PLAYER_STATUS::DEAD;
            }
            if (tempstatus == PLAYER_STATUS::LANDEDRESPAWN && self->status == PLAYER_STATUS::RESPAWN) {
                std::cout << "server said we could respawn!" << std::endl;
                int tmpbs = FindRespawnBase(self->team);

                // Base found, reset the player's speed etc.
                self->Respawn();
                self->Update();
                // mount /dev/player /mnt/Base
                self->x = bases[tmpbs].x;
                self->y = bases[tmpbs].y - 26;

                self->status = PLAYER_STATUS::LANDEDRESPAWN;
            }
            if (tempstatus == PLAYER_STATUS::FLYING && self->status == PLAYER_STATUS::LIFTOFF) {
                std::cout << "we are flying!" << std::endl;
                self->status = PLAYER_STATUS::FLYING;
            }
            if (tempstatus == PLAYER_STATUS::LANDED && self->status == PLAYER_STATUS::FLYING) {
                std::cout << "we have landed!" << std::endl;
                self->status = PLAYER_STATUS::LANDED;
                self->vx = 0;
                self->vy = 0;
                self->engine_on = 0;
                self->flamestate = 0;
            }
            if (tempstatus == PLAYER_STATUS::LANDEDBASE && self->status == PLAYER_STATUS::FLYING) {
                std::cout << "we have landed on a Base!" << std::endl;
                int tmpbase = GetNearestBase(int(self->x), int(self->y));

                self->y = bases[tmpbase].y - 26;
                self->angle = 0;
                self->shipframe = 0;
                self->status = PLAYER_STATUS::LANDEDBASE;
                self->vx = 0;
                self->vy = 0;
                self->engine_on = 0;
                self->flamestate = 0;
                self->Update();
            }
        }
    }
}

// Update the team state, including the bases
void Client::HandleTeamStates(Message& msg) {
    auto sync = msg.As<SyncTeamStates>();

    red_team.frags = sync->red_kills;
    blue_team.frags = sync->blue_kills;
    red_team.bases = 0;
    blue_team.bases = 0;
    for (int bidx = 0; bidx < MAXBASES; bidx++) {
        if (sync->base_states & (1 << (bidx * 2))) {
            bases[bidx].owner = SHIPZ_TEAM::RED;
            red_team.bases++;
        }
        if (sync->base_states & (1 << (bidx * 2 + 1))) {
            bases[bidx].owner = SHIPZ_TEAM::BLUE;
            blue_team.bases++;
        }
    }
}