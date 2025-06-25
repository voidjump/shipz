#ifndef SHIPZGFX_H
#define SHIPZGFX_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>

#include "common/types.h"
#include "common/player.h"
void Slock(SDL_Surface *screen);
void Sulock(SDL_Surface *screen);
void DrawIMG(SDL_Surface *img, int x, int y);
void DrawIMG(SDL_Surface *img, int x, int y,
                                int w, int h, int x2, int y2);
void InitVid(bool fullscreen);
void DrawPlayer( SDL_Surface * src, Player * play );
void DrawExplosions();
void UpdateScreen();
#endif
