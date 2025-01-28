#include "message.h"

#include <iostream>

#include "event.h"
#include "log.h"
#include "net.h"
#include "request.h"
#include "response.h"
#include "sync.h"

// Serialize based on message type
bool Message::Serialize(Buffer &buffer) {
    switch (this->GetMessageType()) {
        case MessageType::EVENT:
            return static_cast<Event *>(this)->Serialize(&buffer);
            break;
        case MessageType::REQUEST:
            return static_cast<Request *>(this)->Serialize(&buffer);
            break;
        case MessageType::RESPONSE:
            return static_cast<Response *>(this)->Serialize(&buffer);
            break;
        case MessageType::SYNC:
            return static_cast<Sync *>(this)->Serialize(&buffer);
            break;
        default:
            std::cout << "debug: Cannot Serialize: unknown event type"
                      << std::endl;
            return false;
    }
    return true;
}

Message *Message::Deserialize(Buffer &buffer) {
    Message *message = NULL;
    Uint8 header;

    if (buffer.AvailableRead() < 1) {
        // insufficient bytes remaining for message header
        return NULL;
    }

    Uint8 msg_type = buffer.Peek8();
    switch (GetMessageTypeFromHeader(msg_type)) {
        case MessageType::EVENT:
            log::debug("reading EVENT");
            message = Event::Deserialize(&buffer);
            break;
        case MessageType::REQUEST:
            log::debug("reading REQUEST");
            message = Request::Deserialize(&buffer);
            break;
        case MessageType::RESPONSE:
            log::debug("reading RESPONSE");
            message = Response::Deserialize(&buffer);
            break;
        case MessageType::SYNC:
            log::debug("reading SYNC");
            message = Sync::Deserialize(&buffer);
            break;
        default:
            log::debug("cannot deserialize: unknown message type: ",
                       (Uint16)msg_type);
            return NULL;
    }
    return message;
}
