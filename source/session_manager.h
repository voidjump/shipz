#ifndef SHIPZ_SESSION_MANAGER_H
#define SHIPZ_SESSION_MANAGER_H

#include <map>
#include <unordered_set>
#include <SDL3_net/SDL_net.h>

#include "session.h"
#include "message_manager.h"

// Time out sessions after this many seconds
#define MAX_SESSION_AGE 10 * 1000

// ShipzSessionManager organisses sessions for the server
// It constructs new sessions, checks constraints and terminates them
// It also purges stale sessions
struct ShipzSessionManager {
   private:
    // Sessions connected to this server
    static std::map<SDLNet_Address *, ShipzSession*> active_sessions;
    static std::map<ShipzSessionID, ShipzSession*> by_id_map;
    static std::map<ShipzSessionID, SessionMessageManager*> message_managers;

   public:

    // Terminate a session
    void Terminate(ShipzSessionID session_id) {
        for (auto state = active_sessions.begin();
             state != active_sessions.end();) {
            if (state->second->session_id == session_id) {
                delete state->second;
                active_sessions.erase(state);
                by_id_map.erase(session_id);
                return;
            }
        }
    }

    // Purge all stale sessions older than MAX_SESSION_AGE
    static void PurgeStale() {
        uint64_t time = SDL_GetTicks();
        for (auto state = active_sessions.begin();
             state != active_sessions.end();) {
            if (time - state->second->last_active > MAX_SESSION_AGE) {
                log::info("Dropping stale session ", state->second->session_id);
                state = active_sessions.erase(state);
            } else {
                ++state;
            }
        }
    }

    // Obtain a new session for a given address
    static ShipzSession* NewSession(SDLNet_Address *address) {
        // See if the address is already in the current session store
        if (active_sessions.find(address) != active_sessions.end()) {
            log::debug("Already found session for address ",
                       SDLNet_GetAddressString(address));
            return nullptr;
        }

        ShipzSession * session = new ShipzSession(address);
        if(ShipzSession::IsNoneID(session->session_id)) {
            return nullptr;
        }

        log::info("New session ", session->session_id, " started for ",
                  SDLNet_GetAddressString(address));

        active_sessions[address] = session;
        by_id_map[session->session_id] = session;
        return session;
    }
};

#endif