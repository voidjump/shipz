#ifndef SHIPZOTHER_H
#define SHIPZOTHER_H

#include <SDL3/SDL.h>

#include "net.h"
#include "types.h"
#include "base.h"

SDL_Surface * LoadIMG( const char * filename );

void CreateGonLookup();

void InitSDL();

Base * GetNearestBase( int x, int y );

bool CheckForQuitSignal();

void GetTyping(std::string &buffer, uint key, uint mod);

void NewExplosion( int x, int y );

void ClearOldExplosions();

void CleanAllExplosions();

void EndMessage();

SDL_Surface * LoadBMP( const char * filename );

Mix_Chunk * LoadSound( const char * filename );

std::string GetCurrentTime();
#endif
