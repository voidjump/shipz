#ifndef SHIPZ_PACKET_H
#define SHIPZ_PACKET_H
#include <list>
#include "net.h"

#define AES_BLOCKSIZE 16

// A packet contains a series of shipz messages

// Bitmask constants
constexpr uint16_t SIZE_MASK = 0x07FF;   // 11 bits for size (11111111111)
constexpr uint16_t HEADER_MASK = 0x7800; // 4 bits for header (111100000000)
constexpr uint16_t RELIABLE_MASK = 0x8000; // 1 bit for reliable (1000000000000000)

enum class MessageType {
    REQUEST, // Request for server info - Can request this without association
    RESPONSE,  
    UPDATE, // Game update
    EVENT,
    PING,
};

union MessageHeader {
    struct {
        uint16_t size : 11;   // 11 bits for size (max 2047 bytes)
        uint8_t header : 4;   // 4 bits for header type (16 types)
        bool reliable : 1;    // 1 bit for reliability (true/false)
    };
    uint16_t raw;           // Raw 16-bit access for the entire union
};

struct Message {
    MessageHeader header;
    void* data;
};

class Packet : public Buffer {
    private:
    Uint8 iv[AES_BLOCKSIZE];
    void RandomizeIV();

    public:
    // Append a message
    void Append(Message&);
    // Read messages from buffer
    std::list<Message> Read();

    // Encrypt the underlying data using a preshared 256 bit symmetric AES key
    void Encrypt(Uint8 * key);
    // Decrypt the underlying data using a preshared 256 bit symmetric AES key
    void Decrypt(Uint8 * key);
};

#endif