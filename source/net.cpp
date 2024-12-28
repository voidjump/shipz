#include <string.h>
#include <stdint.h>
#include <iostream>
#include <iomanip>  // for std::hex, std::setw, std::setfill

#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>

#include "types.h"
#include "net.h"

Uint16 Read16(void *area) {
    // 'area' is a pointer to the data buffer
    uint8_t *ptr = (uint8_t*)area;

    // Read the 16-bit value in network byte order (big-endian)
    // Network byte order is typically big-endian, so we need to handle that
    Uint16 value = (ptr[0] << 8) | ptr[1];

    // Convert the value from network byte order (big-endian) to host byte order
    return SDL_Swap16(value);  // SDL_Swap16 converts to host byte order
}

// Read a 32 bit value from a buffer
Uint32 Read32(void *area) {
    uint8_t *ptr = (uint8_t*)area;

    Uint32 value = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];

    // Convert the value from network byte order (big-endian) to host byte order
    return SDL_Swap32(value);  // SDL_Swap16 converts to host byte order
}

void Write16(Uint16 value, void *area) {
    // 'area' is a pointer to the memory buffer
    uint8_t *ptr = (uint8_t*)area;

    // Convert the value from host byte order to network byte order (big-endian)
    value = SDL_Swap16(value);  // SDL_Swap16 converts from host to network byte order

    // Write the 16-bit value in network byte order (big-endian)
    ptr[0] = (value >> 8) & 0xFF;  // Most significant byte
    ptr[1] = value & 0xFF;         // Least significant byte
}

void Write32(Uint32 value, void *area) {
    // 'area' is a pointer to the memory buffer
    uint8_t *ptr = (uint8_t*)area;

    // Convert the value from host byte order to network byte order (big-endian)
    value = SDL_Swap32(value);  // SDL_Swap16 converts from host to network byte order

    // Write the 16-bit value in network byte order (big-endian)
    ptr[0] = (value >> 24) & 0xFF;  // Most significant byte
    ptr[1] = (value >> 16) & 0xFF; 
    ptr[2] = (value >> 8) & 0xFF;  
    ptr[3] = value & 0xFF;         // Least significant byte
}

// Function to print raw bytes
void PrintRawBytes(const char* data, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        // Print in hexadecimal format, ensuring 2 digits per byte
        std::cout << std::setw(2) << std::setfill('0')  // Ensure 2-digit hex
                  << std::hex << (0xFF & data[i]) << " ";  // Print byte as hex
    }
    std::cout << std::dec << std::endl;  // Switch back to decimal for any future output
}

// Log a datagram to stdout
void DebugPackage(const char* debug_msg, SDLNet_Datagram * dgram) {
    std::cout << debug_msg << " : ";
    PrintRawBytes((const char*)dgram->buf, dgram->buflen);
}

Buffer::Buffer() {
    this->position = this->data;
    this->length = 0;
}

// Import bytes from source buffer
bool Buffer::ImportBytes(void * source_buffer, size_t n_bytes) {
    if (n_bytes > MAXBUFSIZE) {
        return false;
    }
    // strictly not needed
    this->Clear();
    this->length = n_bytes;
    memcpy((void *)this->data, source_buffer, n_bytes);
    return true;
}

// Clear the buffer and reset position pointer to the first byte
// TODO: Strictly not necessary
void Buffer::Clear() {
	memset( this->data, '\0', sizeof(this->data) );
    this->position = this->data;
    this->length = 0;
}

// Seek the position pointer to an index.
bool Buffer::Seek(uint index) {
    if (index >= this->length) {
        return false;
    }
    this->position = &this->data[index];
    return true;
}

// Write a byte to the buffer. Returns false if write fails
bool Buffer::Write8(Uint8 byte) {
    if ((MAXBUFSIZE - this->length) <= 0) {
        return false;
    }
    *this->position = byte;
    this->position++;
    this->length++;
    return true;
}

// Write a 16 bit integer value to the buffer
bool Buffer::Write16(Uint16 value) {
    if ((MAXBUFSIZE - this->length) <= 1) {
        return false;
    }
    // Convert the value from host byte order to network byte order (big-endian)
    value = SDL_Swap16(value);  // SDL_Swap16 converts from host to network byte order

    // Write the 16-bit value in network byte order (big-endian)
    this->position[0] = (value >> 8) & 0xFF;  // Most significant byte
    this->position[1] = value & 0xFF;         // Least significant byte
    this->position += 2;
    this->length += 2;
    return true;
}

bool Buffer::Write32(Uint32 value) {
    if ((MAXBUFSIZE - this->length) <= 3) {
        return false;
    }

    // Convert the value from host byte order to network byte order (big-endian)
    value = SDL_Swap32(value);  // SDL_Swap16 converts from host to network byte order

    // Write the 16-bit value in network byte order (big-endian)
    this->position[0] = (value >> 24) & 0xFF;  // Most significant byte
    this->position[1] = (value >> 16) & 0xFF; 
    this->position[2] = (value >> 8) & 0xFF;  
    this->position[3] = value & 0xFF;         // Least significant byte
    this->position += 4;
    this->length += 4;
    return true;
}

// Write a string to the buffer (Does not have to be nul terminated)
bool Buffer::WriteOctets(const char * source, size_t string_length) {
    if((MAXBUFSIZE - this->length) < string_length) {
        return false;
    }
	strncpy( (char *)&this->position, source, string_length ); 
    this->position += string_length;
    this->length += string_length;
    return true;
}

// Write a string to the buffer, without specifying length (Null terminated only)
bool Buffer::WriteString(const char* source) {
    size_t string_length = strlen(source); // Get string length
    if ((MAXBUFSIZE - this->length) < string_length) {
        return false;  // Not enough space left in the buffer
    }

    // Copy the string data into the buffer
    memcpy((void *)this->position, (void *)source, string_length);

    // Move position forward by string_length
    this->position += string_length;
    this->length += string_length;

    return true;
}

// Return how much space is available in the buffer
uint Buffer::Available() {
    return MAXBUFSIZE-this->length;
}

// Read 8 and increment buffer
Uint8 Buffer::Read8() {
    Uint8 retval = (Uint8)*this->position;
    this->position++;
    return retval;
}

// Read a 16 bit value from the buffer
Uint16 Buffer::Read16() {
    // Read the 16-bit value in network byte order (big-endian)
    // Network byte order is typically big-endian, so we need to handle that
    Uint16 value = (this->position[0] << 8) | this->position[1];
    this->position += 2;

    // Convert the value from network byte order (big-endian) to host byte order
    return SDL_Swap16(value);  // SDL_Swap16 converts to host byte order
}

// Read a 32 bit value from the buffer
Uint32 Buffer::Read32() {
    Uint32 value = (this->position[0] << 24) | (this->position[1] << 16) | (this->position[2] << 8) | this->position[3];
    this->position += 4;

    // Convert the value from network byte order (big-endian) to host byte order
    return SDL_Swap32(value);  // SDL_Swap32 converts to host byte order
}

// Read a string from the buffer and copy it into the destination buffer.
// TODO: Safety
void Buffer::ReadStringCopyInto(void * destination_buffer, size_t max_read) {
	strncpy( (char *)destination_buffer, (char *)&this->position, max_read); 
}

// Decrease the position by n. Will not increase beyond position 0
void Buffer::DecreasePosition(uint n) {
    if( n >= this->length ) {
        this->Clear();
    } else {
        this->length -= n;
        this->position = this->data + length; 
    }
}

// Return contents of the buffer as string
const char * Buffer::AsString() {
    return (const char *)this->data;
}

// Write a number of bytes of the same value
void Buffer::WriteBytes(size_t number, Uint8 value) {
    for(int i=0; i < number; i++) {
        *this->position = value;
        this->position++;
        this->length++;
    }
}

