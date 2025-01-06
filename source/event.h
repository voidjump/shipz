#ifndef SHIPZ_EVENT_H
#define SHIPZ_EVENT_H

#include <iostream>
#include <SDL3/SDL.h>
#include <string>

#include "types.h"
#include "net.h"
#include "message.h"

enum EVENT_TYPE {
    PLAYER_JOINS,
    PLAYER_LEAVES,
    TEAM_WINS,
    LEVEL_CHANGE, 
    SERVER_QUIT
};

// A game event
class Event : public Message {
    public:
        virtual ~Event() {
            // Destructor implementation (even if empty)
        }
        // Serialize event into a buffer - should return false if buffer too small
        bool Serialize(Buffer *);
        // Deserialize buffer into a event
        static Event* Deserialize(Buffer *);
};

//      Base class  Class name   Header
#define MESSAGES_EVENTS \
    MESSAGE_CLASS(Event, PlayerJoins,   PLAYER_JOINS,   EVENT) \
    MESSAGE_CLASS(Event, PlayerLeaves,  PLAYER_LEAVES,  EVENT) \
    MESSAGE_CLASS(Event, TeamWins,      TEAM_WINS,      EVENT) \
    MESSAGE_CLASS(Event, LevelChanges,  LEVEL_CHANGE,   EVENT) \
    MESSAGE_CLASS(Event, ServerQuits,   SERVER_QUIT,    EVENT) \

#define FIELDS_PlayerJoins(field_handler) \
    field_handler(FIELD_UINT8, player_number)\
    field_handler(FIELD_STRING, player_name)

#define FIELDS_PlayerLeaves(field_handler) \
    field_handler(FIELD_UINT8, player_number)\
    field_handler(FIELD_STRING, leave_reason)

#define FIELDS_TeamWins(field_handler) \
    field_handler(FIELD_UINT8, team)\

#define FIELDS_LevelChanges(field_handler) \
    field_handler(FIELD_STRING, filename)\
    field_handler(FIELD_STRING, message)

#define FIELDS_ServerQuits(field_handler) \
    field_handler(FIELD_STRING, message)

#define FIELD_UINT8_TYPE Uint8
#define FIELD_UINT16_TYPE Uint16
#define FIELD_UINT32_TYPE Uint32
#define FIELD_STRING_TYPE std::string
#define FIELD_UINT8_SIZE(value) 1
#define FIELD_UINT16_SIZE(value) 2
#define FIELD_UINT32_SIZE(value) 4
#define FIELD_STRING_SIZE(value) value.length() + 1
#define FIELD_UINT8_MINSIZE 1
#define FIELD_UINT16_MINSIZE 2
#define FIELD_UINT32_MINSIZE 4
#define FIELD_STRING_MINSIZE 1
#define FIELD_UINT8_SERIALIZE(value) Write8(value)
#define FIELD_UINT16_SERIALIZE(value) Write16(value)
#define FIELD_UINT32_SERIALIZE(value) Write32(value)
#define FIELD_STRING_SERIALIZE(value) WriteString(value.c_str())
#define FIELD_UINT8_DESERIALIZE Read8()
#define FIELD_UINT16_DESERIALIZE Read16()
#define FIELD_UINT32_DESERIALIZE Read32()
#define FIELD_STRING_DESERIALIZE ReadString()

// Field handler that expands field definition into variable declarations;
#define MESSAGE_FIELD_DECLARATION(type, name) type##_TYPE name;

// Field handler that expands field definition into parameter type list;
#define MESSAGE_FIELD_ARGUMENT_LIST(type, name) type##_TYPE,

// Field handler that expands field definition into parameter name list;
#define MESSAGE_FIELD_ARGUMENT_NAME_LIST(type, name) name,

// Field handler that expands field definition into constructor initializer statements;
#define MESSAGE_FIELD_INIT(type, name) this->name = name;

// Field handler that expands field definition into buffer writes;
#define SERIALIZE_FIELDS_TO_BUFFER(type, name) \
    buffer->type##_SERIALIZE(name);

// Field handler that expands field definition into a required write size;
#define SUM_REQUIRED_FIELD_SIZE(type, name) \
    type##_SIZE(name) +

// Field handler that expands field definition into buffer reads;
#define DESERIALIZE_FIELDS_FROM_BUFFER(type, name) \
    if( buffer->AvailableRead() < type##_MINSIZE ) { \
        return NULL; \
    } \
    type##_TYPE name = buffer->type##_DESERIALIZE;


#define MESSAGE_CLASS(BASE_CLASS, CLASS_NAME, HEADER, BASE_HEADER) \
class BASE_CLASS##CLASS_NAME : public BASE_CLASS{ \
    public: \
        /* Add field declarations */ \
        FIELDS_##CLASS_NAME(MESSAGE_FIELD_DECLARATION) \
        \
        /* Constructor */ \
        BASE_CLASS##CLASS_NAME(FIELDS_##CLASS_NAME(MESSAGE_FIELD_ARGUMENT_LIST) ...) {\
            this->SetMessageType(MessageType::BASE_HEADER); \
            this->SetMessageSubType(HEADER); \
            FIELDS_##CLASS_NAME(MESSAGE_FIELD_INIT) \
        }\
        \
        /* Serialize Message */ \
        bool Serialize(Buffer *buffer); \
        \
        /* Deserialize Message*/ \
        static BASE_CLASS* Deserialize(Buffer *buffer); \
};

MESSAGES_EVENTS
#undef MESSAGE_CLASS

// Implement Serialization methods
#define MESSAGE_CLASS(BASE_CLASS, CLASS_NAME, HEADER, BASE_HEADER) \
    bool BASE_CLASS##CLASS_NAME::Serialize(Buffer *buffer) { \
        if( buffer->AvailableWrite() < ( FIELDS_##CLASS_NAME(SUM_REQUIRED_FIELD_SIZE) 0) ) { \
            return false; \
        } \
        FIELDS_##CLASS_NAME(SERIALIZE_FIELDS_TO_BUFFER) \
        return true;\
    }

MESSAGES_EVENTS
#undef MESSAGE_CLASS

// Implement Deserialization methods
#define MESSAGE_CLASS(BASE_CLASS, CLASS_NAME, HEADER, BASE_HEADER) \
    BASE_CLASS* BASE_CLASS##CLASS_NAME::Deserialize(Buffer *buffer) { \
        if(buffer->AvailableRead() < 1) { \
            return NULL; \
        } \
        /* Consume event type header */ \
        buffer->Read8(); \
        /* Deserialize fields */ \
        FIELDS_##CLASS_NAME(DESERIALIZE_FIELDS_FROM_BUFFER) \
        return new BASE_CLASS##CLASS_NAME(FIELDS_##CLASS_NAME(MESSAGE_FIELD_ARGUMENT_NAME_LIST) NULL); \
    }

MESSAGES_EVENTS
#undef MESSAGE_CLASS

#define MESSAGE_CLASS(BASE_CLASS, CLASS_NAME, HEADER, BASE_HEADER) \
    case HEADER:\
        return static_cast<BASE_CLASS##CLASS_NAME*>(this)->Serialize(buffer); \
        break; \


// Implement Deserialization from base class
bool Event::Serialize(Buffer * buffer) {
    switch(this->GetMessageSubType()) {
        MESSAGES_EVENTS
        default:
        std::cout << "debug: Unknown event type" << std::endl;
        return false;
    }
    return false;
}
#undef MESSAGE_CLASS
#define MESSAGE_CLASS(BASE_CLASS, CLASS_NAME, HEADER, BASE_HEADER) \
    case HEADER: \
        event = BASE_CLASS##CLASS_NAME::Deserialize(buffer); \
        break; \

// Factory function to Deserialize an event
// Returns an event type based on the type 
// of event deserialized
Event* Event::Deserialize(Buffer *buffer) {
    Event *event = NULL;

    switch(GetSubMessageTypeFromHeader(buffer->Peek8())) {
        MESSAGES_EVENTS
        default:
            std::cout << "debug: Unknown event type" << std::endl;
            return NULL;
    }
    return event;
}


#endif