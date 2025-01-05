#ifndef SHIPZ_GLOBALS_H
#define SHIPZ_GLOBALS_H

extern SDL_Window * sdlWindow;
extern SDL_Renderer *sdlRenderer;

// Holds the entire screen
extern SDL_Texture * sdlScreenTexture;
extern SDL_Surface * temp;
extern SDL_Surface * screen;

extern int viewportx;
extern int viewporty;

extern bool shipcolmap[36][28][28];
extern float look_sin[360],
       look_cos[360],
       newtime,
       oldtime,
       deltatime,
       lastsendtime;
 

extern Bullet bullets[NUMBEROFBULLETS];
 
extern Explosion explosions[NUMBEROFEXPLOSIONS];
#endif 