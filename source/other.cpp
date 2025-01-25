#include <SDL3/SDL.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3_image/SDL_image.h>
#include <math.h>
#include <string.h>

#include <fstream>
#include <iostream>
#include <map>

#include "assets.h"
#include "base.h"
#include "level.h"
#include "log.h"
#include "net.h"
#include "player.h"
#include "sound.h"
#include "team.h"
#include "types.h"
#include "chat.h"

std::map<unsigned int, char> upper_case_keys = {
    {SDLK_PERIOD, '>'}, {SDLK_COMMA, '<'}, {SDLK_SEMICOLON, ':'},
    {SDLK_SPACE, ' '},  {SDLK_MINUS, '_'}, {SDLK_APOSTROPHE, '\"'},
    {SDLK_SLASH, '?'},  {SDLK_0, ')'},     {SDLK_1, '!'},
    {SDLK_2, '@'},      {SDLK_3, '#'},     {SDLK_4, '$'},
    {SDLK_5, '%'},      {SDLK_6, '^'},     {SDLK_7, '&'},
    {SDLK_8, '*'},      {SDLK_9, '('}};

std::map<unsigned int, char> lower_case_keys = {
    {SDLK_PERIOD, '.'}, {SDLK_COMMA, ','}, {SDLK_SEMICOLON, ';'},
    {SDLK_SPACE, ' '},  {SDLK_MINUS, '-'}, {SDLK_APOSTROPHE, '\''},
    {SDLK_SLASH, '/'},  {SDLK_0, '0'},     {SDLK_1, '1'},
    {SDLK_2, '2'},      {SDLK_3, '3'},     {SDLK_4, '4'},
    {SDLK_5, '5'},      {SDLK_6, '6'},     {SDLK_7, '7'},
    {SDLK_8, '8'},      {SDLK_9, '9'}};

// Load an image from the /gfx/ directory
// Returns surface containing loaded image
// TODO: Handle load failure?
SDL_Surface *LoadIMG(const char *filename) {
    std::cout << "@ loading asset " << filename << std::endl;
    // Append filename to path
    char tmppath[256];
    memset(tmppath, '\0', sizeof(tmppath));
    snprintf(tmppath, 256, "%s./gfx/%s", SHAREPATH, filename);

    SDL_Surface *loaded_image;
    loaded_image = IMG_Load(tmppath);
    if (loaded_image == NULL) {
        std::cout << "Could not load image " << filename << std::endl;
        SDL_Quit();
    }
    return loaded_image;
}

// Load Bitmap file and convert it's bitmap format
SDL_Surface *LoadBMP(const char *filename) {
    SDL_Surface *surface, *converted_map;
    char tmpfilename[100];

    memset(tmpfilename, '\0', sizeof(tmpfilename));
    snprintf(tmpfilename, 100, "%s./gfx/%s", SHAREPATH, filename);

    surface = SDL_LoadBMP(tmpfilename);
    if (surface == NULL) {
        std::cout << "Could not load image " << filename << std::endl;
        SDL_Quit();
    }
    converted_map = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA8888);

    // SDL_DestroySurface(surface);
    return converted_map;
}

void CreateGonLookup()  // create lookup tables for sin and cos
{
    int i = 0;
    for (i = 0; i <= 359; i++) {
        double theta = double(i * (M_PI / 180));  // convert the values
        look_sin[i] = float(sin(theta));
        look_cos[i] = float(cos(theta));
    }
    look_cos[90] = 0;  // somehow these values aren't converted correctly so we
                       // have to input them manually
    look_sin[180] = 0;
    look_cos[270] = 0;
}

void InitSDL() {
    if (!SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO)) {
        log::error("Unable to init SDL:", SDL_GetError());
        exit(1);
    }
    if (!SDLNet_Init()) {
        log::error("failed to initialize SDLNet", SDL_GetError());
        SDLNet_Quit();
        exit(1);
    }
    // always do SDL_Quit, whatever happens.
    atexit(SDL_Quit);
}

// returns a pointer to the Base nearest to x,y, or NULL if there are none.
Base * GetNearestBase(int x, int y) {
    int tdx, tdy;       
    float nearest_dist, current_dist;
    Base *current_nearest = NULL;           

    for( auto & base : Base::all_bases ) {
        tdx = x - base->x;
        tdy = y - base->y;

        current_dist = sqrt((tdx * tdx) + (tdy * tdy));
        if (current_dist < nearest_dist || current_nearest == NULL) {
            nearest_dist = current_dist;
            current_nearest = base;
        }
    }
    return current_nearest;
}

void NewExplosion(int x, int y) {
    int i;
    for (i = 0; i < NUMBEROFEXPLOSIONS; i++) {
        if (explosions[i].used == 0) {
            explosions[i].used = 1;
            explosions[i].x = x;
            explosions[i].y = y;
            explosions[i].starttime = SDL_GetTicks();
            break;
        }
    }
    PlaySound(explodesound);
}

void ClearOldExplosions() {
    int i;
    for (i = 0; i < NUMBEROFEXPLOSIONS; i++) {
        if (explosions[i].used == 1) {
            float dt = SDL_GetTicks() - explosions[i].starttime;
            if (dt > EXPLOSIONLIFE) {
                explosions[i].used = 0;
                explosions[i].x = 0;
                explosions[i].y = 0;
                explosions[i].starttime = 0;
            } else {
                explosions[i].frame = int(dt) / EXPLOSIONFRAMETIME;
                if (explosions[i].frame == 10) {
                    explosions[i].frame = 9;
                }
            }
        }
    }
}

void CleanAllExplosions() {
    int i;
    for (i = 0; i < NUMBEROFEXPLOSIONS; i++) {
        explosions[i].used = 0;
        explosions[i].x = 0;
        explosions[i].y = 0;
        explosions[i].starttime = 0;
    }
}

bool CheckForQuitSignal() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            return 1;
        }
        if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.key == SDLK_ESCAPE) {
                return 1;
            }
        }
    }
    return 0;
}

// Add typed keys to a buffer
void GetTyping(std::string &buffer, uint key, uint mod) {
    bool shift = 0;
    bool caps = 0;
    if (mod & SDL_KMOD_LSHIFT || mod & SDL_KMOD_RSHIFT) {
        shift = 1;
    }
    if (mod & SDL_KMOD_CAPS) {
        caps = 1;
    }

    if (key == SDLK_BACKSPACE) buffer.pop_back();

    if (buffer.length() > MAX_CHAT_LENGTH) return;

    // a-z / A-Z
    if (key >= SDLK_A && key <= SDLK_Z) {
        Uint8 typed = (Uint8)'a' + (key - SDLK_A);

        if ((shift && !caps) || caps) {
            typed = typed & (1 << 6);
        }
        buffer.push_back(static_cast<char>(typed));
        return;
    }

    // end a-z / A-Z
    if (shift) {
        if (upper_case_keys.count(key) == 1) {
            buffer.push_back(static_cast<char>(upper_case_keys[key]));
        }
    } else {
        if (lower_case_keys.count(key) == 1) {
            buffer.push_back(static_cast<char>(lower_case_keys[key]));
        }
    }
}

void EndMessage() {
    std::cout << std::endl << std::endl;
    std::cout << "Thank you for playing shipz." << std::endl;
    std::cout << "Game code and graphics by Steve van Bennekom." << std::endl;
    std::cout << "Scripts, makefiles and code maintainance by Raymond Jelierse"
              << std::endl;
    std::cout << std::endl;
}

// Attempt to load a .wav file from /sound/ dir and return a Mix_Chunk
Mix_Chunk *LoadSound(const char *filename) {
    Mix_Chunk *loaded_chunk;

    // Append .wav file to /sound/ dir
    char tempstr[200];
    memset(tempstr, '\0', sizeof(tempstr));
    snprintf(tempstr, 200, "%s/sound/%s", SHAREPATH, filename);

    loaded_chunk = Mix_LoadWAV(tempstr);
    if (!loaded_chunk) {
        std::cout << "Mix_LoadWAV: " << SDL_GetError() << std::endl;
        return NULL;
    }
    return loaded_chunk;
}
