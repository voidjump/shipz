#ifndef SHIPZ_MESSAGE_MANAGER_H
#define SHIPZ_MESSAGE_MANAGER_H

#include <deque>
#include <list>
#include "message.h"
#include "packet.h"
#include "session.h"

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
    // The session that this instance handles messages for
    ShipzSession *session;
    
    // Inbound, any channel
    std::deque<std::shared_ptr<Message>> inbound;
    MessageSequenceID last_seen_id;

    // Outbound, reliable
    std::deque<std::shared_ptr<Message>> out_reliable;
    // Next ID
    MessageSequenceID id_counter;

    // Outbound, unreliable
    std::deque<std::shared_ptr<Message>> out_lossy;

    // Constructor
    SessionMessageManager(ShipzSession *session);

    // Write a message to an outbound queue
    void Write(std::shared_ptr<Message> msg);
    // Read all messages in the inbound queue
    std::list<std::shared_ptr<Message>> Read();

    // Read all messages to the inbound queue
    void HandleReceivedPacket(Packet &pack);

    // Write a packet from the inbound queue
    std::unique_ptr<Packet> CraftSendPacket();
};

#endif