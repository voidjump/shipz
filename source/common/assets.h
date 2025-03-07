#ifndef SHIPZ_ASSETS_H
#define SHIPZ_ASSETS_H

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>

void LoadAssets();

// gfx
extern SDL_Surface * crosshairred;
extern SDL_Surface * crosshairblue;
extern SDL_Surface * bulletpixmap;
extern SDL_Surface * rocketpixmap;
extern SDL_Surface * minepixmap;
extern SDL_Surface * chatpixmap;
extern SDL_Surface * explosionpixmap;
extern SDL_Surface * rocket_icon;
extern SDL_Surface * bullet_icon;
extern SDL_Surface * mine_icon;
extern SDL_Surface * scores;
extern SDL_Surface * shipred;
extern SDL_Surface * shipblue;
extern SDL_Surface * level;
extern SDL_Surface * basesimg;

// sounds
extern Mix_Chunk * explodesound;
extern Mix_Chunk * rocketsound;
extern Mix_Chunk * weaponswitch;

//font
extern TTF_Font * sansbold;
extern TTF_Font * sansboldbig;

#endif