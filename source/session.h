#ifndef SHIPZ_SESSION_H
#define SHIPZ_SESSION_H

#include <unordered_set>
#include "message_factory.h"

using ShipzSessionID = uint16_t;
constexpr ShipzSessionID NO_SHIPZ_SESSION = 0;
constexpr uint16_t MAX_SESSION_COUNT = 1024;

class ShipzSession {
   public:
    // tick time this was last active
    uint64_t last_active;
    // The session ID
    ShipzSessionID session_id;
    // The endpoint
    SDLNet_Address *endpoint;

    // Construct a session for an endpoint, allocating an ID
    ShipzSession(SDLNet_Address *endpoint) {
        this->endpoint = endpoint;
        this->last_active = SDL_GetTicks();
        this->session_id = _getFreeID();
        if (IsNoneID(this->session_id)) {
            throw std::runtime_error(
                "Cannot start session, ran out of sessions!.");
        }
        LockID(this->session_id);
    }

    // Construct a session for an endpoint, having a certian ID
    ShipzSession(SDLNet_Address *endpoint, ShipzSessionID session_id) {
        this->endpoint = endpoint;
        this->last_active = SDL_GetTicks();
        if (IsActiveID(session_id)) {
            throw std::runtime_error(
                "Cannot start session, ID already in use.");
        }
        this->session_id = session_id;
        LockID(this->session_id);
    }

    ~ShipzSession() {
        UnlockID(this->session_id);
    }

    // Get the total amount of sessions
    static uint16_t GetSessionCount() {
        return active_ids.size();
    }

    // Whether the max session count has been reached
    static bool Saturated() {
        return GetSessionCount() >= MAX_SESSION_COUNT;
    }

    // Return whether this ID is 'not defined'
    inline static bool IsNoneID(ShipzSessionID session_id) {
        return (session_id == NO_SHIPZ_SESSION);
    }

    // Return if this ID is an active session ID
    inline static bool IsActiveID(ShipzSessionID session_id) {
        return (active_ids.find(session_id) != active_ids.end());
    }
   private:

    // All session ID's currently in use
    static std::unordered_set<ShipzSessionID> active_ids;

    // Obtain a free ID
    static ShipzSessionID _getFreeID() {
        for (ShipzSessionID id = 1; id <= (MAX_SESSION_COUNT *2); id++) {
            if (!IsActiveID(id)) {
                return id;
            }
        }
        return NO_SHIPZ_SESSION;
    }

    // Mark an ID as Used
    inline static void LockID(ShipzSessionID session_id) {
        active_ids.insert(session_id);
    }

    // Mark an ID as Unused
    inline static void UnlockID(ShipzSessionID session_id) {
        active_ids.erase(session_id);
    }
};

/*
 * Session message: packet authentication
 *
 */

#undef BASE_CLASS_NAME
#undef BASE_CLASS_HEADER
#undef MESSAGE_CLASS_LIST
#define BASE_CLASS_NAME Session
#define BASE_CLASS_HEADER SESSION

#define MESSAGE_CLASS_LIST(class_handler)          \
    class_handler(RequestSession, REQUEST_SESSION) \
        class_handler(ProvideSession, PROVIDE_SESSION)

// Request a session (from client)
#define FIELDS_RequestSession(field_handler) \
    field_handler(FIELD_UINT8, version) field_handler(FIELD_UINT16, port)

// Server provides session id for client session
#define FIELDS_ProvideSession(field_handler) \
    field_handler(FIELD_UINT16, session_id)

MESSAGE_FACTORY_HEADER

#endif
