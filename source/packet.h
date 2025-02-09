#ifndef SHIPZ_PACKET_H
#define SHIPZ_PACKET_H
#include <functional>
#include <list>
#include <map>


#include "message.h"
#include "net.h"
#include "log.h"
#include "session.h"

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
    std::list<std::shared_ptr<Message>> Read();

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

// Registry for callbacks that operate on messages
class MessageHandler {
    private:
        // Registry of functions
        std::map<Uint16, std::function<void(std::shared_ptr<Message>)>> registry;

        // The default function to call
        std::function<void(std::shared_ptr<Message>)> default_callback;

        // Holds the origin address when handling packets
        SDLNet_Address * current_origin;

    public:
        // Register a callback function
        void RegisterHandler(std::function<void(std::shared_ptr<Message>)> callback, Uint16 msg_sub_type);

        // Register a default callback function
        void RegisterDefault(std::function<void(std::shared_ptr<Message>)> callback);

        // Delete a packet handler
        void DeleteHandler(Uint16 msg_sub_type);

        // Clear all callbacks
        void Clear();

        // Handle all messages in a packet
        void HandlePacket(Packet &pack);

        // Get the origin address for the current packet 
        SDLNet_Address * CurrentOrigin();
};
#endif