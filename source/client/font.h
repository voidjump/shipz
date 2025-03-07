#ifndef SHIPZFONT_H
#define SHIPZFONT_H

#include <SDL3_ttf/SDL_ttf.h>

enum FONT_COLOR {
    BLACK,
    WHITE,
    YELLOW
};

void InitFont();
TTF_Font * LoadFont( const char * fontname, int size );
void DrawFont( TTF_Font * font, const char * string, int x, int y, int color );
#endif
