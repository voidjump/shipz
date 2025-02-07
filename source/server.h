#ifndef SHIPZSERVER_H
#define SHIPZSERVER_H

#include <unordered_set>

#include "chat.h"
#include "event.h"
#include "net.h"
#include "packet.h"
#include "player.h"
#include "session.h"
#include "socket.h"
#include "team.h"
#include "types.h"

#define SERVER_RUNSTATE_OK 1
#define SERVER_RUNSTATE_FAIL 2
#define SERVER_RUNSTATE_QUIT 3

#define IDLETIMEBEFOREDROP 2000

// Time out sessions after this many seconds
#define MAX_SESSION_AGE 10 * 1000

enum SERVER_SESSION_STATE {
    INIT = 1,
    ACTIVE = 2,
};

struct SessionState {
   private:
    // Sessions connected to this server
    static std::map<SDLNet_Address *, SessionState> active_sessions;
    static std::unordered_set<ShipzSession> ids_in_use;

    // Obtain a free ID
    static ShipzSession _getFreeID() {
        for (ShipzSession id = 1; id < 65535; id++) {
            if (ids_in_use.find(id) == ids_in_use.end()) {
                return id;
            }
        }
        return NO_SHIPZ_SESSION;
    }

   public:
    // tick time this was last active
    uint64_t last_active;
    // The session ID
    ShipzSession session;
    // The state of this session
    uint8_t state;

    SessionState(uint64_t last_active, ShipzSession session, uint8_t state) {
        this->last_active = last_active;
        this->session = session;
        this->state = state;
        ids_in_use.insert(session);
    }

    ~SessionState() {
        ids_in_use.erase(this->session);
        log::info("Terminated session ", this->session);
    }

    // Terminate a session
    void Terminate() {
        for (auto state = active_sessions.begin();
             state != active_sessions.end();) {
            if (&state->second == this) {
                active_sessions.erase(state);
                return;
            }
        }
    }

    // Purge all stale sessions older than MAX_SESSION_AGE
    static void PurgeStale() {
        uint64_t time = SDL_GetTicks();
        for (auto state = active_sessions.begin();
             state != active_sessions.end();) {
            if (time - state->second.last_active > MAX_SESSION_AGE) {
                log::info("Dropping stale session ", state->second.session);
                state = active_sessions.erase(state);
            } else {
                ++state;
            }
        }
    }

    // Obtain a new session for a given address
    static ShipzSession NewSession(SDLNet_Address *address) {
        // See if the address is already in the current session store
        if (active_sessions.find(address) != active_sessions.end()) {
            log::debug("Already found session for address ",
                       SDLNet_GetAddressString(address));
            return NO_SHIPZ_SESSION;
        }
        // insert the session in the map
        auto id = _getFreeID();
        if (id == NO_SHIPZ_SESSION) {
            log::error("Max sessions reached!");
            return NO_SHIPZ_SESSION;
        }

        active_sessions.emplace(
            address,
            SessionState(SDL_GetTicks(), id, SERVER_SESSION_STATE::INIT));
        log::info("New session ", id, " started for ",
                  SDLNet_GetAddressString(address));
        return id;
    }
};

class Server {
   private:
    bool done = false;

    uint max_clients;
    std::string level_name;
    Socket socket;
    MessageHandler handler;
    ChatConsole console;

   public:
    // Start server
    Server(std::string level_name, uint16_t port, uint max_clients);
    ~Server();
    void HandleUnknownMessage(Message *msg);
    void SetupCallbacks();
    void HandleJoin(Message *msg);
    void HandleInfo(Message *msg);
    void CreateSession(Message *msg);

    void Run();
    void Init();
    bool Load();
    void GameLoop();
};

#endif
