#ifndef SHIPZ_MESSAGE_H
#define SHIPZ_MESSAGE_H
#include <SDL3/SDL.h>

#include "net.h"

// Bitmask constants

constexpr Uint8 MESSAGE_SUBTYPE_MASK =
    0b00011111;                                  // 5 bits for message subtype
constexpr Uint8 MESSAGE_TYPE_MASK = 0b01100000;  // 2 bits for message type
constexpr Uint8 RELIABLE_MASK = 0b10000000;      // 1 bit for reliable

enum class MessageType {
    REQUEST = 1,   // Request for data
    RESPONSE = 2,  // Response to data request
    SYNC = 3,      // An update that synchronizes game state
    EVENT = 4,     // An event
};

/// @brief Helper function to return a SubType from a message header
/// @param header header byte to process
/// @return SubType
inline Uint8 GetSubMessageTypeFromHeader(Uint8 header) {
    return ((header & MESSAGE_SUBTYPE_MASK));
}

/// @brief Helper function to return MessageType from a message header
/// @param header header byte to process
/// @return MessageType
inline MessageType GetMessageTypeFromHeader(Uint8 header) {
    return (MessageType)((header & MESSAGE_TYPE_MASK) >> 5);
}

/// @brief Helper function to return MessageType from a message header
/// @param header header byte to process
/// @return MessageType
inline Uint8 ConstructHeader(MessageType message_type, Uint8 sub_type) {
    return (0x00 | (sub_type & MESSAGE_SUBTYPE_MASK) |
            ((Uint8)message_type << 5 & MESSAGE_TYPE_MASK));
}

// A message is a base class for objects that read from and write to a buffer
class Message {
    friend class Packet;

   protected:
    // Header contains the following information:
    // Whether the message is reliable
    // The MessageType
    // The MessageSubType
    Uint8 header;

   public:
    virtual ~Message() = default;  // virtual destructor to enable RTTI

    // Serialize this message into a buffer
    bool Serialize(Buffer &buffer);
    // Deserialize current buffer position into a message
    static Message *Deserialize(Buffer &buffer);

    inline void SetMessageSubType(Uint8 msg_type) {
        this->header = this->header & ~MESSAGE_SUBTYPE_MASK;  // clear bits
        this->header = this->header | ((Uint8)msg_type & MESSAGE_SUBTYPE_MASK);
    }

    inline void SetMessageType(MessageType msg_type) {
        this->header = this->header & ~MESSAGE_TYPE_MASK;  // clear bits
        this->header =
            this->header | (((Uint8)msg_type << 5) & MESSAGE_TYPE_MASK);
    }

    inline MessageType GetMessageType() {
        return GetMessageTypeFromHeader(this->header);
    }

    inline Uint8 GetMessageSubType() {
        return GetSubMessageTypeFromHeader(this->header);
    }

    inline Uint8 GetFullType() { return this->header & ~RELIABLE_MASK; }

    inline bool IsTypes(MessageType type, Uint8 sub_type) {
        return (GetMessageTypeFromHeader(this->header) == type &&
                GetSubMessageTypeFromHeader(this->header) == sub_type);
    }

    inline void SetReliability(bool reliability) {
        this->header = (this->header & ~RELIABLE_MASK) | (reliability << 8);
    }

    inline bool GetReliability() { return (bool)header & RELIABLE_MASK; }

    template <typename T>
    // Cast message to type
    inline T *As() {
        return dynamic_cast<T *>(this);
    }
};

#endif