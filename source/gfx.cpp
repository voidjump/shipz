#include <stdlib.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <iostream>

#include "types.h"
#include "player.h"
#include "team.h"
#include "base.h"
#include "assets.h"
#include "level.h"

// shipz drawing functions

// this function is for the creating of collision map. it checks whether a pixel is black or white.
// returns black = true, white = false
bool GetPixel(SDL_Surface *surf, int x, int y)
{ 
  Uint8 red = 0;

  if( !SDL_ReadSurfacePixel(surf, x, y, &red, NULL, NULL, NULL) )
  {
	std::cout << "failed to retreive pixel value: " << SDL_GetError() << std::endl;
	SDL_Quit();
	exit(1);
  }
  return (red == 0);
}

void Slock(SDL_Surface *screen)
{
  if ( SDL_MUSTLOCK(screen) )
  {
    if ( !SDL_LockSurface(screen) )
    {
      return;
    }
  }
}

void Sulock(SDL_Surface *screen)
{
  if ( SDL_MUSTLOCK(screen) )
  {
    SDL_UnlockSurface(screen);
  }
}

void DrawIMG(SDL_Surface *img, int x, int y)
{
  SDL_Rect dest;
  dest.x = x;
  dest.y = y;
  SDL_BlitSurface(img, NULL, screen, &dest);
}

void DrawIMG(SDL_Surface *img, int x, int y,
                                int w, int h, int x2, int y2)
{
	SDL_Rect dest;
	dest.x = x;
	dest.y = y;
	SDL_Rect dest2;
	dest2.x = x2;
	dest2.y = y2;
	dest2.w = w;
	dest2.h = h;
	SDL_BlitSurface(img, &dest2, screen, &dest);
}

// Initialize video and hide the cursor
void InitVid()
{
	if( !SDL_CreateWindowAndRenderer("shipz", 0, 0, SDL_WINDOW_FULLSCREEN, &sdlWindow, &sdlRenderer))
	{
		std::cout << "Unable to create window and renderer" << std::endl << SDL_GetError() << std::endl;
		exit(1);
	}
	if ( sdlWindow == NULL || sdlRenderer == NULL )
	{
		std::cout << "Unable to create window and renderer" << std::endl << SDL_GetError() << std::endl;
		exit(1);
	}
	if( !SDL_SetRenderLogicalPresentation(sdlRenderer, XRES, YRES, SDL_LOGICAL_PRESENTATION_LETTERBOX))
	{
		std::cout << "Unable to set logical presentation" << std::endl << SDL_GetError() << std::endl;
		exit(1);
	}

	sdlScreenTexture = SDL_CreateTexture(sdlRenderer,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               XRES, YRES);
	screen = SDL_CreateSurface(640, 480, SDL_PIXELFORMAT_ARGB8888);
	
	SDL_HideCursor();
}

void DrawPlayer( SDL_Surface * src, Player * play )
{
	DrawIMG(src, int(play->x - 14 - viewportx), int(play->y -14 - viewporty), 28, 28, play->x_bmp, play->y_bmp);
	if( play->status == PLAYER_STATUS::FLYING && (SDL_GetTicks() - play->lastliftofftime) > LIFTOFFSHOOTDELAY  )
	{
		if( play->team == SHIPZ_TEAM::RED )
		{
			DrawIMG(crosshairred, (int(play->x + play->crossx - 4 -viewportx)),
				(int(play->y + play->crossy - 4 - viewporty)));
		}
		if( play->team == SHIPZ_TEAM::BLUE )
		{
			DrawIMG(crosshairblue, (int(play->x + play->crossx - 4 -viewportx)),
				(int(play->y + play->crossy - 4 - viewporty)));
		}
	}
	if( play->status != PLAYER_STATUS::DEAD && play->status != PLAYER_STATUS::RESPAWN )
	{
		if( play->typing == 1 ) // player is typing, show the cartoony text cloud
		{
			DrawIMG(chatpixmap, int(play->x - 38 - viewportx), int(play->y - 27 - viewporty));
		}
	}
}

void UpdateScreen() {
	SDL_UpdateTexture(sdlScreenTexture, NULL, screen->pixels, screen->pitch);
	SDL_RenderClear(sdlRenderer);
	SDL_RenderTexture(sdlRenderer, sdlScreenTexture, NULL, NULL);
	SDL_RenderPresent(sdlRenderer);
}

void DrawExplosions()
{
	int i, x;
	for( i = 0; i < NUMBEROFEXPLOSIONS; i++ )
	{
		if( explosions[i].used )
		{
			x = explosions[i].frame;
			x = x * 60 + x + 1;
			DrawIMG(explosionpixmap, (explosions[i].x - 30 - viewportx),
							(explosions[i].y -30 - viewporty ), 60, 60, x, 1);
		}
		
	}
}
