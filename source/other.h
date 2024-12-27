#ifndef SHIPZOTHER_H
#define SHIPZOTHER_H

#include <SDL3/SDL.h>

#include "net.h"
#include "types.h"

SDL_Surface * LoadIMG( const char * filename );

void CreateGonLookup();

void InitSDL();

bool LoadLevelFile();

int GetNearestBase( int x, int y );

bool CheckForQuitSignal();

void GetTyping( Buffer * buffer, uint key, uint mod);

void NewExplosion( int x, int y );

void ClearOldExplosions();

void CleanAllExplosions();

void EndMessage();

Mix_Chunk * LoadSound( const char * filename );
#endif
