#ifndef SHIPZ_MESSAGE_HANDLER_H
#define SHIPZ_MESSAGE_HANDLER_H
#include <functional>
#include <map>

#include "message.h"
#include "log.h"
#include "session.h"

// Registry for callbacks that operate on messages
class MessageHandler {
    private:
        // Registry of functions
        std::map<Uint16, std::function<void(MessagePtr, ShipzSession*)>> registry;

        // The default function to call
        std::function<void(MessagePtr, ShipzSession*)> default_callback;

        // Holds the origin address when handling packets
        SDLNet_Address * current_origin;
    
        // Holds the current session ID we're handling
        ShipzSessionID current_session_id;

    public:
        // Register a callback function
        void RegisterHandler(std::function<void(MessagePtr, ShipzSession*)> callback, Uint16 msg_sub_type);

        // Register a default callback function
        void RegisterDefault(std::function<void(MessagePtr, ShipzSession*)> callback);

        // Delete a packet handler
        void DeleteHandler(Uint16 msg_sub_type);

        // Clear all callbacks
        void Clear();

        // Handle all messages in a packet
        void HandlePacket(Packet &pack);

        // Handle message for a session
        void HandleMessage(MessagePtr msg, ShipzSession* session);

        // Handle messagelist for a session
        void HandleMessageList(MessageList msgs, ShipzSession* session);

        // Get the origin address for the current packet 
        inline SDLNet_Address * CurrentOrigin() {
            return this->current_origin;
        }

        // Get the origin address for the current packet 
        inline ShipzSessionID CurrentSessionID() {
            return this->current_session_id;
        }
};

#endif