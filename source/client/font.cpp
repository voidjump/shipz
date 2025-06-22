#include <iostream>
#include <SDL3_ttf/SDL_ttf.h>
#include "common/types.h"
#include "client/gfx.h"
#include "client/font.h"

void InitFont()
{
	if(!TTF_Init())
	{
		std::cout << "TTF_Init: " << SDL_GetError() << std::endl;
	}
}

TTF_Font * LoadFont( const char * fontname, int size )
{
	TTF_Font * tempfont;
	char tempstr[120];
	snprintf( tempstr, 120, "%s/fonts/%s", SHAREPATH, fontname );
	tempfont = TTF_OpenFont( tempstr, size);
	if(!tempfont) 
	{
		std::cout << "Error loading font " << SDL_GetError() << std::endl;
		return NULL;
	}
	return tempfont;
}

void DrawFont( TTF_Font * font, const char * string, int x, int y, int color )
{
	SDL_Surface * text_surface;
	SDL_Color col; 
	switch( color )
	{
		case FONT_COLOR::BLACK:
			col.r = 0;
			col.b = 0;
			col.g = 0;
			break;
		case FONT_COLOR::WHITE:
			col.r = 255;
			col.b = 255;
			col.g = 255;
			break;
		case FONT_COLOR::YELLOW:
			col.r = 253;
			col.g = 220;
			col.b = 37;
	}
	
	if(!(text_surface=TTF_RenderText_Blended( font, string, strlen(string), col ))) 
	{
		std::cout << "Error loading font " << SDL_GetError() << std::endl;
	}
	DrawIMG( text_surface, x, y );
	SDL_DestroySurface( text_surface );
}
