#include <SDL3/SDL.h>
#include "common/types.h"

SDL_Window * sdlWindow;
SDL_Renderer *sdlRenderer;

// Holds the entire screen
SDL_Texture * sdlScreenTexture;
SDL_Surface * temp;
SDL_Surface * screen;

int viewportx = 0;
int viewporty = 0;

bool shipcolmap[36][28][28];
float look_sin[360],
       look_cos[360];
 
Explosion explosions[NUMBEROFEXPLOSIONS];