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
SDL_Surface * temp;
SDL_Surface * screen;

int viewportx=0;
int viewporty=0;

bool shipcolmap[36][28][28];
float look_sin[360],
       look_cos[360],
       newtime = 0,
       oldtime = 0,
       deltatime = 0,
       lastsendtime = 0;
 

Bullet bullets[NUMBEROFBULLETS];
 
Explosion explosions[NUMBEROFEXPLOSIONS];


// MAIN ///////////////////////////////////////////////////////////////////////////////////////////////////////

int main( int argc, char *argv[] )
{
	bool run_server = false; // is this the server or a client?
	bool cli_init = false;
	int error = 0, menu_result;

	char client_ip[16];
	char client_nick[13];
	char * level_filename;
	
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
				cli_init = true;
				run_server = true;
				level_filename = argv[2];
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
				cli_init = true;
				run_server = false;
				strcpy( client_ip, argv[2] );
				strcpy( client_nick, argv[3] );
			}
		}
	}

	if( cli_init && error == 0 )
	{
		InitSDL();
		if( run_server )
		{
			Server server(level_filename);
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
