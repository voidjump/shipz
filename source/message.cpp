#include <iostream>
#include "message.h"
#include "net.h"
#include "event.h"
#include "response.h"
#include "request.h"
#include "sync.h"
#include "log.h"


// Serialize based on message type
bool Message::Serialize(Buffer &buffer) {

    switch(this->GetMessageType()) {
        case MessageType::EVENT:
            return static_cast<Event*>(this)->Serialize(&buffer);
            break;
        case MessageType::REQUEST:
            return static_cast<Request*>(this)->Serialize(&buffer);
            break;
        case MessageType::RESPONSE:
            return static_cast<Response*>(this)->Serialize(&buffer);
            break;
        case MessageType::SYNC:
            return static_cast<Sync*>(this)->Serialize(&buffer);
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
        // insufficient bytes remaining for message header
        return NULL;
    }
     
    switch(GetMessageTypeFromHeader(buffer.Peek8())) {
        case MessageType::EVENT:
            log::debug("reading EVENT");
            message = (Message *)Event::Deserialize(&buffer);
            break;
        case MessageType::REQUEST:
            log::debug("reading REQUEST");
            message = (Message *)Request::Deserialize(&buffer);
            break;
        case MessageType::RESPONSE:
            log::debug("reading RESPONSE");
            message = (Message *)Response::Deserialize(&buffer);
            break;
        case MessageType::SYNC:
            log::debug("reading SYNC");
            message = (Message *)Sync::Deserialize(&buffer);
            break;
        default:
            std::cout << "debug: Cannot Deserialize: Unknown message type" << std::endl;
            return NULL;
    }
    return message;
}
