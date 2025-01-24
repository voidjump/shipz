#ifndef SHIPZGFX_H
#define SHIPZGFX_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>

#include "types.h"
#include "player.h"

bool GetPixel(SDL_Surface *screen, int x, int y);
void Slock(SDL_Surface *screen);
void Sulock(SDL_Surface *screen);
void DrawIMG(SDL_Surface *img, int x, int y);
void DrawIMG(SDL_Surface *img, int x, int y,
                                int w, int h, int x2, int y2);
void InitVid();
void DrawPlayer( SDL_Surface * src, Player * play );
void DrawExplosions();
void UpdateScreen();
#endif
