#include <random>
#include "packet.h"
#include "AES.h"

AES aes_g(AESKeyLength::AES_256); 
std::random_device rand_dev_g;

// Append a message to a packet, serializing it's contents to its buffer
void Packet::Append(Message &msg) {
    msg.Serialize(static_cast<Buffer&>(*this));
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