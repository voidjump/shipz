#ifndef SHIPZFONT_H
#define SHIPZFONT_H
void InitFont();
TTF_Font * LoadFont( const char * fontname, int size );
void DrawFont( TTF_Font * font, const char * string, int x, int y, int color );
#endif
