#include <iostream>
#include "assets.h"
#include "other.h"
#include "font.h"
#include "level.h"

// gfx
SDL_Surface * crosshairred;
SDL_Surface * crosshairblue;
SDL_Surface * bulletpixmap;
SDL_Surface * rocketpixmap;
SDL_Surface * minepixmap;
SDL_Surface * chatpixmap;
SDL_Surface * explosionpixmap;
SDL_Surface * rocket_icon;
SDL_Surface * bullet_icon;
SDL_Surface * mine_icon;
SDL_Surface * scores;
SDL_Surface * shipred;
SDL_Surface * shipblue;
SDL_Surface * level;
SDL_Surface * basesimg;

// sounds:
Mix_Chunk * explodesound;
Mix_Chunk * rocketsound;
Mix_Chunk * weaponswitch;

// font
TTF_Font * sansbold;
TTF_Font * sansboldbig;

void LoadAssets() {
	std::cout << "@ loading data" << std::endl;
	shipred = LoadIMG( "red.png" );
	shipblue = LoadIMG( "blue.png" );
	chatpixmap = LoadIMG( "chatting.png" );
	level = LoadIMG( lvl.m_image_filename.c_str() );
	crosshairred = LoadIMG( "crosshairred.png" );
	crosshairblue = LoadIMG( "crosshairblue.png" );
	bulletpixmap = LoadIMG( "bullet.png" );
	rocketpixmap = LoadIMG( "rocket.png" );
	minepixmap = LoadIMG( "mines.png" );
	basesimg = LoadIMG( "bases.png" );
	explosionpixmap = LoadIMG( "explosions.png" );
	rocket_icon = LoadIMG( "rocket_icon.png" );
	bullet_icon = LoadIMG( "bullet_icon.png" );
	mine_icon = LoadIMG( "mine_icon.png" );
	scores = LoadIMG( "scores.png" );

	
	explodesound = LoadSound( "boom.wav" );
	rocketsound = LoadSound( "rocket.wav" );
	weaponswitch = LoadSound( "weapon_switch.wav" );

	sansbold = LoadFont( "sansbold.ttf", 12 );
	sansboldbig = LoadFont( "Beware.ttf", 16 );
}