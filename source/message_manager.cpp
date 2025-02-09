#include "message_manager.h"

SessionMessageManager::SessionMessageManager(ShipzSession *session) {
    this->session = session;
}

// Write a message to an outbound queue
void SessionMessageManager::Write(std::shared_ptr<Message> msg) {
    if(msg->GetReliability()) {
        this->out_reliable.push_front(msg);
    } else {
        this->out_lossy.push_front(msg);
    }
}
// Read all messages in the inbound queue
std::list<std::shared_ptr<Message>> SessionMessageManager::Read() {
    std::list<std::shared_ptr<Message>> list;
    return list;
}

// Read all messages to the inbound queue
void SessionMessageManager::HandleReceivedPacket(Packet &pack) {
    auto messages = pack.Read();
    // for(auto msg:messages) {

    // }

}

// Write a packet from the inbound queue
std::unique_ptr<Packet>  SessionMessageManager::CraftSendPacket() {
    return nullptr;

}