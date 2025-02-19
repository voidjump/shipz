#ifndef SHIPZ_MESSAGE_H
#define SHIPZ_MESSAGE_H
#include <SDL3/SDL.h>

#include <bitset>
#include <list>
#include <string>
#include "net.h"

// Bitmask constants
class Message;
using MessagePtr = std::shared_ptr<Message>;
using MessageList = std::list<MessagePtr>;
using MessageSequenceID = uint8_t;

constexpr int MAX_MESSAGE_SEQUENCE_ID = 2^(8*sizeof(MessageSequenceID));
constexpr Uint8 MESSAGE_SUBTYPE_MASK =
    0b00001111;                                  // 4 bits for message subtype
constexpr Uint8 MESSAGE_TYPE_MASK = 0b01110000;  // 3 bits for message type
constexpr Uint8 RELIABLE_MASK = 0b10000000;      // 1 bit for reliable

enum class MessageType {
    REQUEST = 1,   // Request for data
    RESPONSE = 2,  // Response to data request
    SYNC = 3,      // An update that synchronizes game state
    EVENT = 4,     // An event
    SESSION = 5,   // Session data
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
    return (MessageType)((header & MESSAGE_TYPE_MASK) >> 4);
}

/// @brief Helper function to return MessageType from a message header
/// @param header header byte to process
/// @return MessageType
inline Uint8 ConstructHeader(MessageType message_type, Uint8 sub_type) {
    return (0x00 | (sub_type & MESSAGE_SUBTYPE_MASK) |
            ((Uint8)message_type << 4 & MESSAGE_TYPE_MASK));
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
    // The sequence number (used for reliable message tracking)
    Uint8 seq_nr;

   public:
    virtual ~Message() = default;  // virtual destructor to enable RTTI

    // return byte size
    virtual int Size() const = 0;

    // Serialize this message into a buffer
    bool Serialize(Buffer &buffer);

    // Deserialize current buffer position into a message
    static MessagePtr Deserialize(Buffer &buffer);

    inline void SetMessageSubType(Uint8 msg_type) {
        this->header = this->header & ~MESSAGE_SUBTYPE_MASK;  // clear bits
        this->header = this->header | ((Uint8)msg_type & MESSAGE_SUBTYPE_MASK);
    }

    inline void SetMessageType(MessageType msg_type) {
        this->header = this->header & ~MESSAGE_TYPE_MASK;  // clear bits
        this->header =
            this->header | (((Uint8)msg_type << 4) & MESSAGE_TYPE_MASK);
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
        this->header = (this->header & ~RELIABLE_MASK) | (reliability << 7);
    }

    inline bool GetReliability() { return (bool)(header & RELIABLE_MASK); }

     // Getter for seq_nr
    inline int GetSeqNr() {
        return seq_nr;
    }

    // Setter for seq_nr
    inline void SetSeqNr(uint8_t value) {
        seq_nr = value;
    }

    template <typename T>
    // Cast message to type
    inline T *As() {
        return dynamic_cast<T *>(this);
    }

    inline std::string DebugHeader() {
        return std::bitset<8>(header).to_string();
    }

    // Get a message as a debug string
    virtual std::string AsDebugStr() = 0;
};

#endif