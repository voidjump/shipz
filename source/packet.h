#ifndef SHIPZ_PACKET_H
#define SHIPZ_PACKET_H
#include "message.h"
#include "net.h"
#include "log.h"
#include "types.h"

#define AES_BLOCKSIZE 16

// This should be changed if any thing is placed before the dynamic buffer
constexpr uint16_t PACKET_HEADER_SIZE = sizeof(ShipzSessionID);

// A packet is a buffer containing a series of shipz messages
// It owns the memory that the messages write to and read from
class Packet : public Buffer {
   private:
    Uint8 iv[AES_BLOCKSIZE];
    void RandomizeIV();

   public:
    ShipzSessionID session;
    SDLNet_Address * origin;
    SDLNet_Address * destination;

    
    void Append(std::shared_ptr<Message> message) {
        if (message) {
            message->Serialize(*this);
        }
    }

    // Append a message
    template <typename T>
    void Append(T &message) {
        static_assert(std::is_base_of<Message, typename std::remove_pointer<T>::type>::value,
                  "T must derive from Message");

        if constexpr (std::is_pointer<T>::value) {
            message->Serialize(dynamic_cast<Buffer*>(this));  // Use -> for pointers
            message->LogDebug();
        } else {
                message.Serialize(dynamic_cast<Buffer*>(this));   // Use . for non-pointers
            message.LogDebug();
        }

        log::debug("pack.Append: ", this->AsHexString());
    }


    // Read messages from buffer
    MessageList Read();

    // Read the packet session
    inline ShipzSessionID ReadSession() {
        this->session = this->Read16();
        return this->session;
    }

    Packet();
    Packet(ShipzSessionID session);
    ~Packet();

    inline ShipzSessionID SessionID() {
        return this->session;
    }

    // Encrypt the underlying data using a preshared 256 bit symmetric AES key
    void Encrypt(Uint8* key);
    // Decrypt the underlying data using a preshared 256 bit symmetric AES key
    void Decrypt(Uint8* key);
};

#endif