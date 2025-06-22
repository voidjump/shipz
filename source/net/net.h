#ifndef SHIPZ_NET_H
#define SHIPZ_NET_H

#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>
#include <cmath>
#include <string>
#include <vector>
#define MAXBUFSIZE 1024

// Utilities for converting floating point precision to 16 bits:

// Convert Signed Float value to 16 bit value
inline Uint16 SignedFloatToNet(float value) {
    // Scale by 4 and ensure it fits into the range of an int16_t
    int16_t scaled = static_cast<int16_t>(std::round(value * 8));
    // Reinterpret the int16_t as a uint16_t for storage
    return static_cast<uint16_t>(scaled);
}

// Convert Signed Float value to 16 bit value
inline Uint16 UnsignedFloatToNet(float value) {
    // Reverse the scaling
    return static_cast<uint16_t>(value * 8);
}

// Convert Sint16 value to Signed Float
inline float NetToSignedFloat(Uint16 value) {
    // Reinterpret the uint16_t as an int16_t to recover the signed value
    int16_t signedValue = static_cast<int16_t>(value);
    // Divide by 4 to undo the scaling
    return static_cast<float>(signedValue) / 8.0f;
}

// Convert Sint16 value to Signed Float
inline float NetToUnsignedFloat(Uint16 value) {
    // Direct scaling without clamping or normalization
    return static_cast<uint16_t>(value) / 8.0f;
}

Uint16 Read16(void *area);
Uint32 Read32(void *area);
void Write16(Uint16 value, void *area);
void Write32(Uint32 value, void *area);
void PrintRawBytes(const char* data, size_t length);

enum SHIPZ_MESSAGE {
    CHAT,
    STATUS,
    KICK,
    LEAVE,
    UPDATE,
    MSG_PLAYER_JOINS,
    MSG_PLAYER_LEAVES,
    JOIN,
    EVENT,
};

// TODO: Move to buffer.h, use underlying stringtream
class Buffer {
    protected:
        Uint8 * position;
    public:
        Uint16 length; // amount of data currently in the buffer
        Uint8 data[MAXBUFSIZE];
    Buffer();

    bool ImportBytes(void *, size_t);

    // Clear the buffer
    void Clear();
    // Seek to a location
    bool Seek(Uint16);
    // Write a byte
    bool Write8(Uint8);
    bool Write8(Uint8, const char *);
    // Write 2 bytes
    bool Write16(Uint16);
    bool Write16(Uint16, const char *);
    // Write 4 bytes
    bool Write32(Uint32);
    bool Write32(Uint32, const char *);
    // Write a null terminated string
    bool WriteString(const char *);
    // Write bytes
    bool WriteOctets(std::vector<Uint8> data);

    Uint16 AvailableWrite();
    Uint16 AvailableRead();

    // Read 8 bytes
    Uint8 Read8();
    Uint8 Peek8();
    Uint8 Read8(const char *);
    // Read 16 bytes
    Uint16 Read16();
    Uint16 Peek16();
    Uint16 Read16(const char *);
    // Read 32 bytes
    Uint32 Read32();
    Uint32 Read32(const char *);
    // Read into new string
    std::string& ReadString();
    // Read octets
    std::vector<Uint8> ReadOctets(size_t size);
    // Decrease the position pointer
    void DecreasePosition(Uint16 n);
    // Return buffer as string
    const char * AsString();
    // Write similar bytes to buffer
    void WriteBytes(size_t number, Uint8 value);
    // Set a byte at a Position
    void SetPosByte(Uint16 pos, Uint8 value);
    // Output buffer for debugging
    void OutputDebug();
    // return buffer as raw bytes 
    std::string AsHexString();
};

#endif
