#include "session.h"

#include <unordered_set>

std::unordered_set<ShipzSessionID> ShipzSession::active_ids;
std::map<ShipzSessionID, ShipzSession *> ShipzSession::by_id_map;

SessionMessageManager::SessionMessageManager(ShipzSession *session) {
    this->session = session;
    this->last_seen_id = 255;
}

// Write a message to an outbound queue
void SessionMessageManager::Write(MessagePtr msg) {
    // log::debug(":: header: ", msg->DebugHeader());

    if (msg->GetReliability()) {
        msg->SetSeqNr(id_counter);
        id_counter = (id_counter + 1) % MAX_MESSAGE_SEQUENCE_ID;
        this->out_reliable.push_front(msg);
        log::debug("@", session->session_id, " ==> ",  msg->AsDebugStr(), " ", out_reliable.size() );
    } else {
        this->out_lossy.push_front(msg);
        log::debug("@", session->session_id, " --> ",  msg->AsDebugStr(), " ", out_lossy.size() );
    }
}

// Read all messages to the inbound queue
// Reliable messages should be acked
// Unreliable messages are simply pushed on the list
// TODO:
// !!!!!!!!!!!!!!!!!!
// We should verify that the reported session ID from the packet actually
// Originates from the endpoint that belongs to the session!!
// This prevents session spoofing
// !!!!!!!!!!!!!!!!!!
void SessionMessageManager::HandleReceivedPacket(Packet &pack) {
    auto messages = pack.Read();
    uint8_t reliable_count = 0;
    this->session->last_active = SDL_GetTicks();
    // log::debug("handling packet for session ", this->session->session_id, ":",
    //            messages.size());

    MessageSequenceID last_seen = this->last_seen_id;
    for (auto msg : messages) {
        if (msg->GetReliability()) {
            if (!is_new_message(msg->GetSeqNr(), this->last_seen_id)) {
                continue;
            }
            // This is a new message and we should ack it.
            last_seen = newest(last_seen, msg->GetSeqNr());
            reliable_count++;
        }

        if (msg->GetMessageType() == MessageType::SESSION) {
            if (msg->GetMessageSubType() == ACKNOWLEDGE) {
                // Drop all messages from the outbound queue that are
                // Older than this ACK
                auto ack = msg->As<SessionAck>();
                log::debug("@", session->session_id, " <-- ACK ", (uint16_t)ack->last_seq_no);
                ProcessAck(ack->last_seq_no);
            }
            // Note all other session messages are ignored!
        } else {
            log::debug("@", session->session_id, " <-- ", msg->AsDebugStr());
            this->inbound.push_back(msg);
        }
    }
    if (reliable_count > 0) {
        // add a new ack message to outbound
        MessagePtr ack = std::make_shared<SessionAck>(last_seen);
        this->last_seen_id = last_seen;
        this->out_lossy.push_back(ack);
    }
}

// Is this a new message, with respect to the last seen message, respecting
// a windowing function of half the message id space
bool SessionMessageManager::is_new_message(MessageSequenceID msg_id,
                                           MessageSequenceID last_seen) {
    int half_range = MAX_MESSAGE_SEQUENCE_ID / 2;  // 128 for 1-byte IDs
    int diff =
        (msg_id - last_seen_id) % MAX_MESSAGE_SEQUENCE_ID;  // Handle wraparound

    if (diff < 0) {
        diff += MAX_MESSAGE_SEQUENCE_ID;  // Ensure diff is always positive
    }

    return (diff > 0 && diff <= half_range);
}

// Windowing max function (Which is the newest id?)
MessageSequenceID SessionMessageManager::newest(MessageSequenceID a,
                                                MessageSequenceID b) {
    if (is_new_message(a, b)) {
        return a;
    } else {
        return b;
    }
}

// Drop all relabiable outbound packages equal or older than ack_id
void SessionMessageManager::ProcessAck(MessageSequenceID ack_id) {
    auto message = out_reliable.begin();

    while (message != out_reliable.end()) {
        uint8_t message_id = message->get()->GetSeqNr();  // Extract message ID

        if (is_acked(last_seen_id, message_id, ack_id)) {
            log::debug("Removing acknowledged message: ", (int)message_id);
            message = out_reliable.erase(message);  // Erase and move to next element
        } else {
            ++message;  // Move to the next message
        }
    }

    last_seen_id = ack_id;  // Update last acknowledged message
}

// Function to determine if a message is acknowledged
bool SessionMessageManager::is_acked(uint8_t last_ack, uint8_t message_id,
                                     uint8_t ack_id) {
    int half_range = MAX_MESSAGE_SEQUENCE_ID / 2;
    int diff = (ack_id - message_id) % MAX_MESSAGE_SEQUENCE_ID;

    if (diff < 0)
        diff += MAX_MESSAGE_SEQUENCE_ID;  // Ensure positive difference

    return diff < half_range;  // If it's within the lower half of the range,
                               // it's acknowledged
}

// Write a packet from the outbound queue
std::unique_ptr<Packet> SessionMessageManager::CraftSendPacket() {
    if (out_reliable.empty() && out_lossy.empty()) {
        return nullptr;  // No messages to send
    }

    // Create a new packet
    auto packet = new Packet(this->session->session_id);

    // Append messages from the outbound queues
    for (MessagePtr message : out_reliable) {
        if (packet) {
            if (packet->AvailableWrite() > message->Size()) {
                packet->Append(message);  // Add message to packet
            }
        }
    }
    for (MessagePtr message : out_lossy) {
        if (packet) {
            if (packet->AvailableWrite() > message->Size()) {
                packet->Append(message);  // Add message to packet
            }
        }
    }
    out_lossy.clear();

    return std::unique_ptr<Packet>(packet);
}

// Return a list of all messages in the inbound queue (depleting it)
MessageList SessionMessageManager::Read() {
    MessageList messages;

    // Move messages from deque to list
    while (!inbound.empty()) {
        messages.push_back(std::move(inbound.front()));
        inbound.pop_front();
    }

    return messages;
}

MESSAGE_FACTORY_IMPLEMENTATION