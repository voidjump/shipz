#include "message_handler.h"

// Register a callback function
//
// The callbacks are registered on the full header, with the exclusion of
// The reliability bit
// So the reliable and unreliable message will both call the same message
void MessageHandler::RegisterHandler(
    std::function<void(MessagePtr, ShipzSession *)> callback, Uint16 header) {
    log::debug("registering handler for ", header);
    header = header & ~RELIABLE_MASK;
    log::debug("after mask removed: ", header);
    this->registry[header] = callback;
}

// Register a default callback function
void MessageHandler::RegisterDefault(
    std::function<void(MessagePtr, ShipzSession *)> callback) {
    this->default_callback = callback;
}

// Delete a packet handler
void MessageHandler::DeleteHandler(Uint16 msg_sub_type) {
    this->registry.erase(msg_sub_type);
}

// Clear all callbacks
void MessageHandler::Clear() { this->registry.clear(); }

// Handle all messages in a packet
void MessageHandler::HandlePacket(Packet &pack) {
    auto messages = pack.Read();
    this->current_origin = pack.origin;
    auto session = ShipzSession::GetSessionById(pack.SessionID());

    for (auto msg : messages) {
        HandleMessage(msg, session);
    }
    this->current_origin = NULL;
}

// Handle message for a session
// 
// Note session pointer can be NULL if no active session matches.
void MessageHandler::HandleMessage(MessagePtr msg, ShipzSession* session) {
    Uint16 msg_type = msg->GetFullType();
    // Check if the registry contains a handler for this message type
    if (this->registry.count(msg_type) == 0) {
        // Call default handler
        this->default_callback(msg, session);
        return;
    }
    // Call registered callback
    this->registry[msg_type](msg, session);
}

// Handle messagelist for a session
// 
// Note session pointer can be NULL if no active session matches.
void MessageHandler::HandleMessageList(MessageList msg_list, ShipzSession* session) {
    for(auto msg:msg_list) {
        HandleMessage(msg, session);
    }
}