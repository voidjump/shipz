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
#define SEND_DELAY 100
#define MAXPLAYERS 8

// physics
#define REALITYSCALE 0.0005
#define SHIPMASS 20
#define GRAVITY 8.00
#define ROTATIONSPEED 400
#define THRUST 784.8
#define BULLETSPEED 100
#define BULLETDELAY 200
#define ROCKETDELAY 500
#define MINEDELAY 2000
#define MINEDETONATERADIUS 50
#define MINELIFETIME 30000
#define MINEACTIVATIONTIME 2000
#define EXPLOSIONLIFE 500
#define EXPLOSIONFRAMETIME 50

// player:
#define LIFTOFFSHOOTDELAY 3000

// internal:
#define NUMBEROFBULLETS 800
#define NUMBEROFEXPLOSIONS 200
#define MAXBASES 16

extern SDL_Surface * temp;
extern SDL_Surface * screen;
extern SDL_Window * sdlWindow;
extern SDL_Texture * sdlScreenTexture;
extern SDL_Renderer * sdlRenderer;

extern int viewportx;
extern int viewporty;

extern bool shipcolmap[36][28][28];
extern float look_sin[360],
       look_cos[360],
       newtime,
       oldtime,
       deltatime,
       lastsendtime;




struct Explosion
{
	bool used;
	int x;
	int y;
	float starttime;
	int frame;
};


extern Bullet bullets[NUMBEROFBULLETS];

extern Explosion explosions[NUMBEROFEXPLOSIONS];

#endif
