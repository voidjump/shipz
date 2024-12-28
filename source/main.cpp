#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <stdarg.h>
#include <time.h>
#include <string.h>

#include <SDL3/SDL.h>

#include "server.h"
#include "client.h"
#include "menu.h"
#include "other.h"
#include "gfx.h"

SDL_Window * sdlWindow;
SDL_Renderer *sdlRenderer;

// Holds the entire screen
SDL_Texture * sdlScreenTexture;

// gfx
SDL_Surface * temp;
SDL_Surface * screen;
SDL_Surface * crosshairred;
SDL_Surface * crosshairblue;
SDL_Surface * bulletpixmap;
SDL_Surface * rocketpixmap;
SDL_Surface * minepixmap;
SDL_Surface * chatpixmap;
SDL_Surface * explosionpixmap;
SDL_Surface * money;
SDL_Surface * rocket_icon;
SDL_Surface * bullet_icon;
SDL_Surface * mine_icon;
SDL_Surface * scores;


// sounds:
Mix_Chunk * explodesound;
Mix_Chunk * rocketsound;
Mix_Chunk * weaponswitch;

// font
TTF_Font * sansbold;
TTF_Font * sansboldbig;

bool iamserver = 0; // is this the server or a client?

int viewportx=0;
int viewporty=0;

bool shipcolmap[36][28][28];
float look_sin[360],
       look_cos[360],
       newtime = 0,
       oldtime = 0,
       deltatime = 0,
       lastsendtime = 0;
 
LevelData lvl;

Bullet bullets[NUMBEROFBULLETS];
 
Base bases[MAXBASES];

Team red_team;
Team blue_team;

Explosion explosions[NUMBEROFEXPLOSIONS];


// MAIN ///////////////////////////////////////////////////////////////////////////////////////////////////////

int main( int argc, char *argv[] )
{
	bool cli_init = 0;
	int error = 0, menu_result;
	red_team.players = 0;
	blue_team.players = 0;

	char client_ip[16];
	char client_nick[13];
	
	if( argc > 1 )
	{
		if( strstr( argv[1], "server" ))
		{
			if( argc < 3 )
			{
				error = 10;
			}
			else
			{
				cli_init = 1;
				iamserver = 1;
				lvl.filename = argv[2];
			}
		}
		if( strstr( argv[1], "client" ))
		{
			if( argc < 4 )
			{
				error = 10;
			}
			else
			{
				cli_init = 1;
				iamserver = 0;
				strcpy( client_ip, argv[2] );
				strcpy( client_nick, argv[3] );
			}
		}
	}

	if( cli_init && error == 0 )
	{
		InitSDL();
		if( iamserver )
		{
			Server server(lvl.filename);
			server.Run();
		}
		else
		{
			InitVid();
			Client client(client_ip, client_nick);
			client.Init();
			client.Run();
		}
	}
	
	SDL_Quit();
	EndMessage();
}
