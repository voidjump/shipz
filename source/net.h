#ifndef SHIPZNET_H
#define SHIPZNET_H

#include <SDL3_net/SDL_net.h>

Uint16 Read16(void *area);
Uint32 Read32(void *area);
void Write16(Uint16 value, void *area);
void Write32(Uint32 value, void *area);
void PrintRawBytes(const char* data, size_t length);
void DebugPackage(const char* debug_msg, SDLNet_Datagram * dgram);


class Buffer {
    private:
        Uint8 * position;
    public:
        Uint16 length; // amount of data currently in the buffer
        Uint8 data[MAXBUFSIZE];
    Buffer();

    bool ImportBytes(void *, size_t);

    // Clear the buffer
    void Clear();
    // Seek to a location
    bool Seek(uint);
    // Write a byte
    bool Write8(Uint8);
    // Write 2 bytes
    bool Write16(Uint16);
    // Write 4 bytes
    bool Write32(Uint32);
    // Write a null terminated string
    bool WriteOctets(const char *, size_t);
    bool WriteString(const char *);

    uint Available();

    // Read 8 bytes
    Uint8 Read8();
    // Read 16 bytes
    Uint16 Read16();
    // Read 32 bytes
    Uint32 Read32();
    // Read string into destination buffer
    void ReadStringCopyInto(void *, size_t);
    // Decrease the position pointer
    void DecreasePosition(uint n);
    // Return buffer as string
    const char * AsString();
    // Write similar bytes to buffer
    void WriteBytes(size_t number, Uint8 value);
    // Set a byte at a Position
    void SetPosByte(Uint16 pos, Uint8 value);
};

#endif
