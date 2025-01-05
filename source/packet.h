#ifndef SHIPZ_PACKET_H
#define SHIPZ_PACKET_H
#include <list>
#include "net.h"
#include "message.h"

#define AES_BLOCKSIZE 16

// A packet is a buffer containing a series of shipz messages
// It owns the memory that the messages write to and read from
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