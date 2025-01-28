#ifndef SHIPZ_MESSAGE_FACTORY_H
#define SHIPZ_MESSAGE_FACTORY_H

// Macros to create message classes
// Aye matey, Here be dragons

#include <SDL3/SDL.h>

#include <iostream>
#include <string>

#include "message.h"
#include "net.h"
#include "log.h"

// Expand field into type
#define FIELD_UINT8_TYPE Uint8
#define FIELD_UINT16_TYPE Uint16
#define FIELD_UINT32_TYPE Uint32
#define FIELD_STRING_TYPE std::string
#define FIELD_OCTETS_TYPE std::vector<Uint8> 
// Expand field into serialization size
#define FIELD_UINT8_SIZE(value) 1
#define FIELD_UINT16_SIZE(value) 2
#define FIELD_UINT32_SIZE(value) 4
#define FIELD_STRING_SIZE(value) value.length() + 1
#define FIELD_OCTETS_SIZE(value) value.size()
// Expand field into minimum deserializatino size
#define FIELD_UINT8_MINSIZE 1
#define FIELD_UINT16_MINSIZE 2
#define FIELD_UINT32_MINSIZE 4
#define FIELD_STRING_MINSIZE 1
#define FIELD_OCTETS_MINSIZE 1
// Expand field into serialization instruction
#define FIELD_UINT8_SERIALIZE(value) Write8(value)
#define FIELD_UINT16_SERIALIZE(value) Write16(value)
#define FIELD_UINT32_SERIALIZE(value) Write32(value)
#define FIELD_STRING_SERIALIZE(value) WriteString(value.c_str())
#define FIELD_OCTETS_SERIALIZE(value) WriteOctets(value)
// Expand field into deserialization instruction
#define FIELD_UINT8_DESERIALIZE Read8()
#define FIELD_UINT16_DESERIALIZE Read16()
#define FIELD_UINT32_DESERIALIZE Read32()
#define FIELD_STRING_DESERIALIZE ReadString()
#define FIELD_OCTETS_DESERIALIZE ReadOctets(size)
// Expand field into log message
#define FIELD_UINT8_DEBUG(name, value) log::debug(name, ":", (uint16_t)value)
#define FIELD_UINT16_DEBUG(name, value) log::debug(name, ":", value)
#define FIELD_UINT32_DEBUG(name, value) log::debug(name, ":", value)
#define FIELD_STRING_DEBUG(name, value) log::debug(name, ":", value)
#define FIELD_OCTETS_DEBUG(name, value) log::debug(name, ":", value.size())

// Indirection macro's
#define CONCAT(a, b) a##b
#define EXPAND_CONCAT(a, b) CONCAT(a, b)

#define REMOVE_FIRST(X, ...) __VA_ARGS__

// Indirection macro required to perform expansion of macro parameters
#define EXPAND_AND_REMOVE_FIRST(...) REMOVE_FIRST(__VA_ARGS__)

#define STRINGIFY(x) #x
#define EXPAND_STRINGIFY(x) STRINGIFY(x)

#define CLASS_LIST_EMIT_HEADER(CLASS_NAME, HEADER) HEADER
#define CLASS_LIST_EMIT_HEADER_COMMA(CLASS_NAME, HEADER) HEADER,

// Field handler that expands field definition into variable declarations;
#define MESSAGE_FIELD_DECLARATION(type, name) type##_TYPE name;

// Field handler that expands field definition into parameter type list;
#define MESSAGE_FIELD_ARGUMENT_LIST(type, name) ,type##_TYPE name

// Field handler that expands field definition into parameter name list;
#define MESSAGE_FIELD_ARGUMENT_NAME_LIST(type, name) ,name

// Field handler that expands field definition into constructor initializer statements;
#define MESSAGE_FIELD_INIT(type, name) this->name = name;

// Field handler that expands field definition into buffer writes;
#define SERIALIZE_FIELDS_TO_BUFFER(type, name) buffer->type##_SERIALIZE(name);

// Field handler that expands field definition into buffer writes;
#define DEBUG_LOG_FIELDS(type, name) type##_DEBUG(#name, name);

// Field handler that expands field definition into a required write size;
#define SUM_REQUIRED_FIELD_SIZE(type, name) type##_SIZE(name) +

// Field handler that expands field definition into buffer reads;
#define DESERIALIZE_FIELDS_FROM_BUFFER(type, name)  \
    if (buffer->AvailableRead() < type##_MINSIZE) { \
        return NULL;                                \
    }                                               \
    type##_TYPE name = buffer->type##_DESERIALIZE;

// Define message classes
#define MESSAGE_CLASS_DEFINITION_HANDLER(CLASS_NAME, HEADER)                                               \
    class EXPAND_CONCAT(BASE_CLASS_NAME, CLASS_NAME) : public BASE_CLASS_NAME {                            \
       public:                                                                                             \
        /* Add field declarations */                                                                       \
        FIELDS_##CLASS_NAME(MESSAGE_FIELD_DECLARATION)                                                     \
                                                                                                           \
            /* Constructor */                                                                              \
            EXPAND_CONCAT(BASE_CLASS_NAME, CLASS_NAME)(EXPAND_AND_REMOVE_FIRST(FIELDS_##CLASS_NAME( MESSAGE_FIELD_ARGUMENT_LIST))) { \
            this->SetMessageType(MessageType::BASE_CLASS_HEADER);                                          \
            this->SetMessageSubType(HEADER);                                                               \
            FIELDS_##CLASS_NAME(MESSAGE_FIELD_INIT)                                                        \
        }                                                                                                  \
                                                                                                           \
        /* Serialize Message */                                                                            \
        bool Serialize(Buffer *buffer);                                                                    \
                                                                                                           \
        /* debug Message */                                                                                \
        void LogDebug();                                                                                  \
                                                                                                           \
        /* Deserialize Message*/                                                                           \
        static BASE_CLASS_NAME *Deserialize(Buffer *buffer);                                               \
    };

// Implement serialization case in base class
#define MESSAGE_CLASS_SERIALIZATION_CASE(CLASS_NAME, HEADER)                                       \
    case HEADER:                                                                                   \
        return static_cast<EXPAND_CONCAT(BASE_CLASS_NAME, CLASS_NAME) *>(this)->Serialize(buffer); \
        break;

// Implement deserialization case in base class
#define MESSAGE_CLASS_DESERIALIZATION_CASE(CLASS_NAME, HEADER)                      \
    case HEADER:                                                                    \
        log::debug("reading", #CLASS_NAME);                                         \
        instance = EXPAND_CONCAT(BASE_CLASS_NAME, CLASS_NAME)::Deserialize(buffer); \
        break;

// Implement Serialization methods
#define MESSAGE_CLASS_SERIALIZATION_FUNCTIONS(CLASS_NAME, HEADER)                          \
    bool EXPAND_CONCAT(BASE_CLASS_NAME, CLASS_NAME)::Serialize(Buffer *buffer) {           \
        if (buffer->AvailableWrite() < (FIELDS_##CLASS_NAME(SUM_REQUIRED_FIELD_SIZE) 0)) { \
            return false;                                                                  \
        }                                                                                  \
        /* Write header */                                                                 \
        buffer->Write8(this->header);                                                      \
        FIELDS_##CLASS_NAME(SERIALIZE_FIELDS_TO_BUFFER) return true;                       \
    }

// Implement Deserialization methods
#define MESSAGE_CLASS_DESERIALIZATION_FUNCTIONS(CLASS_NAME, HEADER)                              \
    BASE_CLASS_NAME *EXPAND_CONCAT(BASE_CLASS_NAME, CLASS_NAME)::Deserialize(Buffer *buffer) {   \
        if (buffer->AvailableRead() < 1) {                                                       \
            return NULL;                                                                         \
        }                                                                                        \
        /* Consume header */                                                                     \
        buffer->Read8();                                                                         \
        /* Deserialize fields */                                                                 \
        FIELDS_##CLASS_NAME(DESERIALIZE_FIELDS_FROM_BUFFER) return dynamic_cast<BASE_CLASS_NAME*>( new EXPAND_CONCAT(            \
            BASE_CLASS_NAME, CLASS_NAME)(EXPAND_AND_REMOVE_FIRST(FIELDS_##CLASS_NAME(MESSAGE_FIELD_ARGUMENT_NAME_LIST)))); \
    }

// Implement debug methods
#define MESSAGE_CLASS_DEBUG_FUNCTIONS(CLASS_NAME, HEADER)           \
    void EXPAND_CONCAT(BASE_CLASS_NAME, CLASS_NAME)::LogDebug() {   \
        FIELDS_##CLASS_NAME(DEBUG_LOG_FIELDS)                       \
    }

// Implement Deserialization from base class
#define BASE_CLASS_SERIALIZATION_IMPLEMENTATION                                                              \
    bool BASE_CLASS_NAME::Serialize(Buffer *buffer) {                                                        \
        switch (this->GetMessageSubType()) {                                                                 \
            MESSAGE_CLASS_LIST(MESSAGE_CLASS_SERIALIZATION_CASE)                                             \
            default:                                                                                         \
                std::cout << "debug: Unknown " << EXPAND_STRINGIFY(BASE_CLASS_NAME) << " type" << std::endl; \
                return false;                                                                                \
        }                                                                                                    \
        return false;                                                                                        \
    }

// Factory function to Deserialize an event
// Returns an event type based on the type
// of event deserialized
#define BASE_CLASS_DESERIALIZATION_IMPLEMENTATION                                                           \
    BASE_CLASS_NAME *BASE_CLASS_NAME::Deserialize(Buffer *buffer) {                                         \
        BASE_CLASS_NAME *instance = NULL;                                                                   \
                                                                                                            \
        switch (GetSubMessageTypeFromHeader(buffer->Peek8())) {                                             \
            MESSAGE_CLASS_LIST(MESSAGE_CLASS_DESERIALIZATION_CASE)                                          \
            default:                                                                                        \
                std::cout << "debug: Unknown " << EXPAND_STRINGIFY(BASE_CLASS_NAME) << "type" << std::endl; \
                return NULL;                                                                                \
        }                                                                                                   \
        return instance;                                                                                    \
    }

#define MESSAGE_TYPE_ENUM_DECLARATION                                                                        \
    enum EXPAND_CONCAT(BASE_CLASS_HEADER, _SUBMESSAGE_TYPE) {                                                \
        MESSAGE_CLASS_LIST(CLASS_LIST_EMIT_HEADER_COMMA) EXPAND_CONCAT(BASE_CLASS_HEADER, _SUBMESSAGE_COUNT) \
    };

#define BASE_CLASS_DECLARATION                         \
    class BASE_CLASS_NAME : public Message {           \
       public:                                         \
        virtual ~BASE_CLASS_NAME() {}                  \
        bool Serialize(Buffer *);                      \
        static BASE_CLASS_NAME *Deserialize(Buffer *); \
    };

// Declare enumeration type
// Declare base class
// Declare message class list
#define MESSAGE_FACTORY_HEADER    \
    MESSAGE_TYPE_ENUM_DECLARATION \
    BASE_CLASS_DECLARATION        \
    MESSAGE_CLASS_LIST(MESSAGE_CLASS_DEFINITION_HANDLER)

#define MESSAGE_FACTORY_IMPLEMENTATION                        \
    BASE_CLASS_SERIALIZATION_IMPLEMENTATION                   \
    BASE_CLASS_DESERIALIZATION_IMPLEMENTATION                 \
    MESSAGE_CLASS_LIST(MESSAGE_CLASS_SERIALIZATION_FUNCTIONS) \
    MESSAGE_CLASS_LIST(MESSAGE_CLASS_DESERIALIZATION_FUNCTIONS)\
    MESSAGE_CLASS_LIST(MESSAGE_CLASS_DEBUG_FUNCTIONS)

#endif