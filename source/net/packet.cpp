#include <random>

#include "net/AES.h"
#include "net/packet.h"
#include "utils/log.h"
#include "net/session.h"

AES aes_g(AESKeyLength::AES_256);
std::random_device rand_dev_g;

Packet::Packet() {
    this->Write16(NO_SHIPZ_SESSION);
    this->session = NO_SHIPZ_SESSION;
}

Packet::Packet(ShipzSessionID session_id) {
    this->Write16(session_id);
    this->session = session_id;
}


// Delete the packet
Packet::~Packet() {
    // logger::debug("packet destroyed");
    // TODO: Find out why this doesn't work
}

// Return a list of messages
MessageList Packet::Read() {
    MessageList messages;
    this->Seek(PACKET_HEADER_SIZE);
    while (true) {
        MessagePtr msg = Message::Deserialize(static_cast<Buffer &>(*this));
        if (msg == NULL) {
            break;
        }
        messages.push_back(msg);
    }
    // logger::debug("read ", messages.size(), " messages");
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
void Packet::Encrypt(Uint8 *key) {
    Uint8 padding =
        (AES_BLOCKSIZE - (this->length % AES_BLOCKSIZE)) % AES_BLOCKSIZE;
    for (int i = 0; i < padding; i++) {
        this->Write8(padding);
    }
    this->RandomizeIV();

    unsigned char *encrypted_payload = aes_g.EncryptCBC(
        (const unsigned char *)this->data, (unsigned int)this->length,
        (const unsigned char *)key, (const unsigned char *)&this->iv);
    memcpy((void *)this->data, (void *)encrypted_payload, this->length);
    delete encrypted_payload;
}

// Decrypt the underlying data using a preshared 256 bit symmetric AES key
// For padding, expect PKCS#7
void Packet::Decrypt(Uint8 *key) {
    unsigned char *decrypted_data = aes_g.DecryptCBC(
        (const unsigned char *)this->data, (unsigned int)this->length,
        (const unsigned char *)key, (const unsigned char *)&this->iv);
    memcpy((void *)this->data, (void *)decrypted_data, this->length);
    delete decrypted_data;
}