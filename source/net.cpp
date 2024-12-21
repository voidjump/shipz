#include <string.h>
#include <stdint.h>

#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>

#include "types.h"

Uint16 Read16(void *area)
{
    // 'area' is a pointer to the data buffer
    uint8_t *ptr = (uint8_t*)area;

    // Read the 16-bit value in network byte order (big-endian)
    // Network byte order is typically big-endian, so we need to handle that
    Uint16 value = (ptr[0] << 8) | ptr[1];

    // Convert the value from network byte order (big-endian) to host byte order
    return SDL_Swap16(value);  // SDL_Swap16 converts to host byte order
}

void Write16(Uint16 value, void *area)
{
    // 'area' is a pointer to the memory buffer
    uint8_t *ptr = (uint8_t*)area;

    // Convert the value from host byte order to network byte order (big-endian)
    value = SDL_Swap16(value);  // SDL_Swap16 converts from host to network byte order

    // Write the 16-bit value in network byte order (big-endian)
    ptr[0] = (value >> 8) & 0xFF;  // Most significant byte
    ptr[1] = value & 0xFF;         // Least significant byte
}
