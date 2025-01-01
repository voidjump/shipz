#ifndef SHIPZ_TYPES_H
#define SHIPZ_TYPES_H

#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>

#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>
#define SHIPZ_VERSION 113

// gfx
#define XRES 640
#define YRES 480 
#define CROSSHAIRDIST 30

// net
#define PORT_SERVER 3500 // The port the server listens on
#define PORT_CLIENT 3501 // The port the client listens on
#define CHATDELAY 1000
#define MAXCHATCHARS 80
#define MAXIDLETIME 1000
#define IDLETIMEBEFOREDROP 2000
#define SENDDELAY 100
#define MAXBUFSIZE 1024
#define MAXPLAYERS 8


// physics
#define REALITYSCALE 0.0005
#define SHIPMASS 20
#define GRAVITY 8.00
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

// weapons:
#define BULLET 11
#define ROCKET 22
#define MINE 33

// player:
#define LIFTOFFSHOOTDELAY 3000


// internal:
#define NUMBEROFBULLETS 800
#define NUMBEROFEXPLOSIONS 200
#define MAXBASES 16


// gfx
extern SDL_Window * sdlWindow;
extern SDL_Texture * sdlScreenTexture;
extern SDL_Renderer * sdlRenderer;
extern SDL_Surface * temp;
extern SDL_Surface * screen;
extern SDL_Surface * crosshairred;
extern SDL_Surface * crosshairblue;
extern SDL_Surface * bulletpixmap;
extern SDL_Surface * rocketpixmap;
extern SDL_Surface * minepixmap;
extern SDL_Surface * chatpixmap;
extern SDL_Surface * explosionpixmap;
extern SDL_Surface * rocket_icon;
extern SDL_Surface * bullet_icon;
extern SDL_Surface * mine_icon;
extern SDL_Surface * scores;

// sounds
extern Mix_Chunk * explodesound;
extern Mix_Chunk * rocketsound;
extern Mix_Chunk * weaponswitch;

//font
extern TTF_Font * sansbold;
extern TTF_Font * sansboldbig;

extern bool iamserver; // is this the server or a client?

extern int viewportx;
extern int viewporty;

extern bool shipcolmap[36][28][28];
extern float look_sin[360],
       look_cos[360],
       newtime,
       oldtime,
       deltatime,
       lastsendtime;

struct LevelData
{
	int width, height; // width and height of the level
	char * filename; // name of the .info file
	char name[80], author[80]; 
	char image[80], colmap[80];
	int levelcode, levelversion; // obsolete | not used yet
	int type, bases; // obsolete | not used yet
};



struct Bullet
{
	bool active; // does the bullet exist?
	float x, y, vx, vy, angle; // coordinates & speed
	int owner; // number of player who shot the bullet
	int type; // type of bullet
	bool collide; // collided flag
	float minelaidtime;
};

struct Explosion
{
	bool used;
	int x;
	int y;
	float starttime;
	int frame;
};

extern LevelData lvl;

extern Bullet bullets[NUMBEROFBULLETS];


extern Explosion explosions[NUMBEROFEXPLOSIONS];

#endif
