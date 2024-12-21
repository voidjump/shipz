#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <stdarg.h>
#include <time.h>
#include <string.h>

#include <SDL3/SDL.h>

#include "error.h"
#include "server.h"
#include "client.h"
#include "menu.h"
#include "other.h"
#include "gfx.h"

#define SHIPZ_VERSION 113

// gfx
#define XRES 640
#define YRES 480 
#define CROSSHAIRDIST 30

// net
#define PORT 3500
#define CHATDELAY 1000
#define MAXCHATCHARS 80
#define MAXIDLETIME 1000
#define IDLETIMEBEFOREDROP 2000
#define SENDDELAY 100
#define MAXBUFSIZE 1024
#define MAXPLAYERS 8

// physics
#define REALITYSCALE 0.005
#define SHIPMASS 20
#define GRAVITY 9.81
#define PI 3.14159
#define ROTATIONSPEED 400
#define THRUST 784.8
#define BULLETSPEED 100
#define BULLETDELAY 200
#define ROCKETDELAY 500
#define ROCKETSPEED 95
#define ROCKETRADARRADIUS 300
#define MINEDELAY 2000
#define MINEDETONATERADIUS 50
#define MINELIFETIME 30000
#define MINEACTIVATIONTIME 2000
#define MAXROCKETTURN 50
#define EXPLOSIONLIFE 500
#define EXPLOSIONFRAMETIME 50

// statuses:
#define DEAD 0
#define FLYING 1
#define LANDED 2
#define JUSTCOLLIDEDROCK 3
#define JUSTCOLLIDEDSHIP 4
#define JUSTSHOT 5
#define RESPAWN 6
#define LIFTOFF 7
#define SUICIDE 8
#define JUSTCOLLIDEDBASE 9
#define LANDEDBASE 10
#define LANDEDRESPAWN 11

// weapons:
#define BULLET 11
#define ROCKET 22
#define MINE 33

// player:
#define STARTING_MONEY 0
#define LIFTOFFSHOOTDELAY 3000

// teams/bases:
#define NEUTRAL 0
#define RED 1
#define BLUE 2

// internal:
#define NUMBEROFBULLETS 800
#define NUMBEROFEXPLOSIONS 200
#define MAXBASES 16

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

bullet bullets[NUMBEROFBULLETS];
 
base bases[MAXBASES];

team red_team;
team blue_team;

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
			error = Server();
		}
		else
		{
			InitVid();
			error = Client( client_ip, client_nick );
		}
	}
	
	ShowError( error );
	SDL_Quit();
	EndMessage();
}
