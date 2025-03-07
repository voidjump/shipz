#ifndef SHIPZ_GLOBALS_H
#define SHIPZ_GLOBALS_H

#include <SDL3/SDL.h>
#include "types.h"

extern SDL_Window * sdlWindow;
extern SDL_Renderer *sdlRenderer;

// Holds the entire screen
extern SDL_Texture * sdlScreenTexture;
extern SDL_Surface * temp;
extern SDL_Surface * screen;

extern int viewportx;
extern int viewporty;

extern bool shipcolmap[36][28][28];
extern float look_sin[360],
       look_cos[360];
 
 
extern Explosion explosions[NUMBEROFEXPLOSIONS];
#endif 