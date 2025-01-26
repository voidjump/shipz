#include <iostream>
#include "message.h"
#include "net.h"
#include "event.h"


// Serialize based on message type
bool Message::Serialize(Buffer &buffer) {

    switch(this->GetMessageType()) {
        case MessageType::EVENT:
            return static_cast<Event*>(this)->Serialize(&buffer);
            break;
        default:
            std::cout << "debug: Cannot Serialize: unknown event type" << std::endl;
            return false;
    }
    return true;
}

Message* Message::Deserialize(Buffer &buffer) {
    Message *message = NULL;
    Uint8 header;

    if( buffer.AvailableRead() < 1) {
        std::cout << "insufficient bytes remaining for message header" << std::endl;
        return NULL;
    }
     
    switch(GetMessageTypeFromHeader(buffer.Peek8())) {
        case MessageType::EVENT:
            message = (Message *)Event::Deserialize(&buffer);
            break;
        default:
            std::cout << "debug: Cannot Deserialize: Unknown event type" << std::endl;
            return NULL;
    }
    return message;
}
