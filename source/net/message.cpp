#include "net/message.h"

#include <iostream>
#include <memory>

#include "messages/event.h"
#include "utils/log.h"
#include "net/net.h"
#include "messages/request.h"
#include "messages/response.h"
#include "net/session.h"
#include "messages/sync.h"

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
        case MessageType::SESSION:
            return static_cast<Session *>(this)->Serialize(&buffer);
            break;
        default:
            std::cout << "debug: Cannot Serialize: unknown event type"
                      << std::endl;
            return false;
    }
    return true;
}

MessagePtr Message::Deserialize(Buffer &buffer) {
    MessagePtr message = NULL;
    Uint8 header;

    if (buffer.AvailableRead() < 1) {
        // insufficient bytes remaining for message header
        return NULL;
    }

    Uint8 msg_type = buffer.Peek8();
    switch (GetMessageTypeFromHeader(msg_type)) {
        case MessageType::EVENT:
            message = std::dynamic_pointer_cast<Message>(Event::Deserialize(&buffer));
            break;
        case MessageType::REQUEST:
            message = std::dynamic_pointer_cast<Message>(Request::Deserialize(&buffer));
            break;
        case MessageType::RESPONSE:
            message = std::dynamic_pointer_cast<Message>(Response::Deserialize(&buffer));
            break;
        case MessageType::SYNC:
            message = std::dynamic_pointer_cast<Message>(Sync::Deserialize(&buffer));
            break;
        case MessageType::SESSION:
            message = std::dynamic_pointer_cast<Message>(Session::Deserialize(&buffer));
            break;
        default:
            logger::debug("cannot deserialize: unknown message type: ",
                       (Uint16)msg_type);
            return NULL;
    }
    return message;
}
