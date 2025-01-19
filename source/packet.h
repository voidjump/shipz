#ifndef SHIPZ_PACKET_H
#define SHIPZ_PACKET_H
#include <functional>
#include <list>
#include <optional>
#include <map>


#include "message.h"
#include "net.h"

#define AES_BLOCKSIZE 16

// Registry for callbacks that operate on messages
class MessageHandler {
    private:
        // Registry of functions
        std::map<Uint16, std::function<void(Message&)>&> registry;

        // The default function to call
        std::function<void(Message&)>& default_callback;

    public:
        // Register a callback function
        void RegisterHandler(std::function<void(Message&)> callback, Uint16 msg_sub_type);

        // Register a default callback function
        void RegisterDefault(std::function<void(Message&)> callback);

        // Delete a packet handler
        void DeleteHandler(Uint16 msg_sub_type);

        // Clear all callbacks
        void Clear();

        // Handle all messages in a packet
        void HandlePacket(Packet &pack);
};

// A packet is a buffer containing a series of shipz messages
// It owns the memory that the messages write to and read from
class Packet : public Buffer {
   private:
    Uint8 iv[AES_BLOCKSIZE];
    void RandomizeIV();

   public:
    // Append a message
    template <typename T>
    void Packet::Append(T& message);

    // Read messages from buffer
    std::list<Message> Read();

    // Encrypt the underlying data using a preshared 256 bit symmetric AES key
    void Encrypt(Uint8* key);
    // Decrypt the underlying data using a preshared 256 bit symmetric AES key
    void Decrypt(Uint8* key);
};

#endif