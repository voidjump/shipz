#ifndef SHIPZNET_H
#define SHIPZNET_H

#include <SDL3_net/SDL_net.h>

Uint16 Read16(void *area);
void Write16(Uint16 value, void *area);

#endif
