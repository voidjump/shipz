#ifndef SHIPZ_SESSION_H
#define SHIPZ_SESSION_H

#include <memory>
#include <map>
#include <unordered_set>
#include <deque>
#include <list>
#include "net/message_factory.h"
#include "net/message.h"
#include "net/packet.h"
#include "common/types.h"

constexpr ShipzSessionID NO_SHIPZ_SESSION = 0;
constexpr uint16_t MAX_SESSION_COUNT = 1024;
class ShipzSession;

// Orchestrates messages for a session.
//
// This class handles out and ingoing messages for a session
// A sender can write messages to the session, and read messages that were
// Received. The manager keeps track of reliable synchronization.
//
// Any messages without a reliable flag set are simply dropped after
// Write() -> Pack() or
// Unpack() -> Read()
//
// Any messages with a reliable flag are tracked:
// Outbound reliable messages end up in the out_reliable queue, and are removed
// if an Ack message is received with a `MessageSequenceID` higher than messages
// in the queue
//
// Inbound reliable messages are only handled if the message_id is higher than
// the `last_seen_id`. Other messages are simply ignored.
//
// In order to deal with looping message sequences the manager uses a windowing
// approach, meaning only half of the message ID space is ever valid at a
// point in time.
class SessionMessageManager {
    private:
    // The session that this instance handles messages for
    ShipzSession *session;

    // Inbound, any channel
    std::deque<MessagePtr> inbound;
    MessageSequenceID last_seen_id;

    // Outbound, reliable
    std::deque<MessagePtr> out_reliable;
    MessageSequenceID last_acked_id;

    // Next ID
    MessageSequenceID id_counter;

    // Outbound, unreliable
    std::deque<MessagePtr> out_lossy;

    // Drop all relabiable outbound packages equal or older than ack_id
    void ProcessAck(MessageSequenceID ack_id);

    // Decide whether a message is newer than another message
    bool is_new_message(MessageSequenceID msg_id, MessageSequenceID last_seen);

    // Decide whether a message has been acked
    bool is_acked(uint8_t message_id, uint8_t ack_id);
    // Return which id is newest
    MessageSequenceID newest(MessageSequenceID a, MessageSequenceID b);

    public:
    // Write a packet from the outbound queue
    std::unique_ptr<Packet> CraftSendPacket();

    // Read all messages to the inbound queue
    void HandleReceivedPacket(Packet &pack);

    // Constructor
    SessionMessageManager(ShipzSession *session);

    // Write a message to an outbound queue
    void Write(MessagePtr msg);

    // Read all messages in the inbound queue
    MessageList Read();
};

class ShipzSession {
   public:
    // tick time this was last active
    uint64_t last_active;
    // tick time we last sent on this session 
    uint64_t last_sent;
    // The session ID
    ShipzSessionID session_id;
    // The client ID (note we reuse the player ID for this)
    ClientID client_id;

    // The endpoint of the counterparty
    SDLNet_Address *endpoint;
    uint16_t port;

    // Message manager for this session
    SessionMessageManager *manager;

    // Construct a session for an endpoint, allocating an ID
    ShipzSession(SDLNet_Address *endpoint, uint16_t port) {
        this->endpoint = endpoint;
        this->port = port;
        this->last_active = SDL_GetTicks();
        this->session_id = _getFreeID();
        if (IsNoneID(this->session_id)) {
            throw std::runtime_error(
                "Cannot start session, ran out of sessions!.");
        }
        Init();
    }

    // Construct a session for an endpoint, having a certian ID
    ShipzSession(SDLNet_Address *endpoint, uint16_t port, ShipzSessionID session_id) {
        this->endpoint = endpoint;
        this->port = port;
        if (IsActiveID(session_id)) {
            throw std::runtime_error(
                "Cannot start session, ID already in use.");
        }
        this->session_id = session_id;
        Init();
    }

    // Initialize state and register to utility maps
    void Init() {
        this->last_active = SDL_GetTicks();
        this->last_sent = 0;
        this->manager = new SessionMessageManager(this);
        by_id_map[session_id] = this;
        LockID(this->session_id);
    }

    ~ShipzSession() {
        UnlockID(this->session_id);
        by_id_map.erase(session_id);
        delete this->manager;
    }

    static ShipzSession* GetSessionById(ShipzSessionID session_id) {
        if(by_id_map.find(session_id) != by_id_map.end()) {
            return by_id_map[session_id];
        }
        return nullptr;
    }

    // Get the total amount of sessions
    static uint16_t GetSessionCount() {
        return active_ids.size();
    }

    // Whether the max session count has been reached
    static bool Saturated() {
        return GetSessionCount() >= MAX_SESSION_COUNT;
    }

    // Does this address / port combination already have a session?
    inline static bool Exists(SDLNet_Address * addr, uint16_t port) {
        for( auto it : by_id_map ) {
            if( it.second->endpoint == addr && it.second->port == port ) {
                return true;
            }
        }
        return false;
    }

    // Return whether this ID is 'not defined'
    inline static bool IsNoneID(ShipzSessionID session_id) {
        return (session_id == NO_SHIPZ_SESSION);
    }

    // Return if this ID is an active session ID
    inline static bool IsActiveID(ShipzSessionID session_id) {
        return (active_ids.find(session_id) != active_ids.end());
    }

    template<typename T, typename... Args>
    void Write(Args&&... args) {
        manager->Write(std::make_shared<T>(std::forward<Args>(args)...));
    }

    bool LastSendGreaterThan(uint64_t ticks) {
        return (SDL_GetTicks() - this->last_sent > ticks);
    }

    void SendTick() {
        this->last_sent = SDL_GetTicks();
    }

   private:
    static std::map<ShipzSessionID, ShipzSession*> by_id_map;

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

// Utility macro
template<typename SessionPtr, typename MessageType, typename... Args>
void WriteSession(SessionPtr session, Args&&... args) {
    session->manager->Write(std::make_shared<MessageType>(std::forward<Args>(args)...));
}

/*
 * Session message: packet authentication
 *
 */

#undef BASE_CLASS_NAME
#undef BASE_CLASS_HEADER
#undef BASE_DEFAULT_RELIABLE
#undef MESSAGE_CLASS_LIST
#define BASE_CLASS_NAME Session
#define BASE_CLASS_HEADER SESSION
#define BASE_DEFAULT_RELIABLE false

// Note all session types are ignored by session message manager in an 
// active session
#define MESSAGE_CLASS_LIST(class_handler)          \
    class_handler(RequestSession, REQUEST_SESSION) \
    class_handler(ProvideSession, PROVIDE_SESSION) \
    class_handler(Ack, ACKNOWLEDGE)

// Request a session (from client)
#define FIELDS_RequestSession(field_handler) \
    field_handler(FIELD_UINT8, version) \
    field_handler(FIELD_UINT16, port)

// Server provides session id for client session
#define FIELDS_ProvideSession(field_handler) \
    field_handler(FIELD_UINT16, session_id)

// Server provides session id for client session
#define FIELDS_Ack(field_handler) \
    field_handler(FIELD_UINT8, last_seq_no)

MESSAGE_FACTORY_HEADER

#endif
