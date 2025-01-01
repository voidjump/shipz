#ifndef SHIPZFONT_H
#define SHIPZFONT_H

enum FONT_COLOR {
    BLACK,
    WHITE,
    YELLOW
};

void InitFont();
TTF_Font * LoadFont( const char * fontname, int size );
void DrawFont( TTF_Font * font, const char * string, int x, int y, int color );
#endif
