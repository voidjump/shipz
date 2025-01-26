#include <random>
#include "packet.h"
#include "AES.h"
#include "log.h"

AES aes_g(AESKeyLength::AES_256); 
std::random_device rand_dev_g;


// Register a callback function
void MessageHandler::RegisterHandler(std::function<void(Message&)> callback, Uint16 msg_sub_type) {
    this->registry[msg_sub_type] = callback;
}

// Register a default callback function
void MessageHandler::RegisterDefault(std::function<void(Message&)> callback) {
    this->default_callback = callback;
}

// Delete a packet handler
void MessageHandler::DeleteHandler(Uint16 msg_sub_type) {
    this->registry.erase(msg_sub_type);
}

// Clear all callbacks
void MessageHandler::Clear() {
    this->registry.clear();
}

// Retrieve the current origion
SDLNet_Address * MessageHandler::CurrentOrigin() {
    return this->current_origin;
}

// Handle all messages in a packet
void MessageHandler::HandlePacket(Packet &pack) {
    log::debug("handling packet");
    auto messages = pack.Read();
    this->current_origin = pack.origin;
    for (Message &msg : messages) {
        Uint16 msg_type = msg.GetMessageSubType();
        // Check if the registry contains a handler for this message type
        if(this->registry.count(msg_type) == 0) {
            // Call default handler
            this->default_callback(msg);
            continue;
        }
        // Call registered callback
        this->registry[msg_type](msg);
    }
    this->current_origin = NULL;
}

// Delete the packet
Packet::~Packet() {
    log::debug("packet destroyed");
    // TODO: Find out why this doesn't work 
    // if(this->origin) SDLNet_UnrefAddress(this->origin);
}

// Return a list of messages
std::list<Message> Packet::Read() {
    std::list<Message> messages;
    this->Seek(0);
    while(true) {
        Message * msg = Message::Deserialize(static_cast<Buffer&>(*this));
        if( msg == NULL) {
            break;
        }
        messages.push_back(*msg);
    }
    return messages;
}

// Randomize the initial vector
void Packet::RandomizeIV() {
    for (int i = 0; i < 16; ++i) {
        iv[i] = static_cast<unsigned char>(rand_dev_g() % 256);
    }
}

// Encrypt the underlying data using a preshared 256 bit symmetric AES key
// For padding, use PKCS#7
void Packet::Encrypt(Uint8 * key) {

    Uint8 padding = (AES_BLOCKSIZE-(this->length % AES_BLOCKSIZE)) % AES_BLOCKSIZE;
    for(int i = 0; i < padding; i++) {
        this->Write8(padding);
    }
    this->RandomizeIV();

    unsigned char* encrypted_payload = aes_g.EncryptCBC((const unsigned char*) this->data, 
                                                        (unsigned int) this->length, 
                                                        (const unsigned char*) key, 
                                                        (const unsigned char*) &this->iv);
    memcpy((void *)this->data, (void *)encrypted_payload, this->length);
    delete encrypted_payload;
}

// Decrypt the underlying data using a preshared 256 bit symmetric AES key
// For padding, expect PKCS#7
void Packet::Decrypt(Uint8 * key) {

    unsigned char* decrypted_data = aes_g.DecryptCBC((const unsigned char*) this->data, 
                                                     (unsigned int) this->length, 
                                                     (const unsigned char*) key, 
                                                     (const unsigned char*) &this->iv);
    memcpy((void *)this->data, (void *)decrypted_data, this->length);
    delete decrypted_data;
}