// sound.cpp
#include <iostream>
#include <SDL3_mixer/SDL_mixer.h>
#include "common/types.h"

const SDL_AudioSpec AUDIO_SPEC = {
	SDL_AUDIO_S16,
	2,
	44100
};

// Initialize SDL Sound and allocate channels
void InitSound()
{
	if(!Mix_OpenAudio(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &AUDIO_SPEC) ){

		std::cout << "Failed to open audio " << SDL_GetError() << std::endl;
	}
	Mix_AllocateChannels(32);
}

// Play a soundchunk
void PlaySound( Mix_Chunk * sound )
{
	if( Mix_PlayChannel(-1, sound, 0) == -1 )
	{
		std::cout << "Could not play sound: " << SDL_GetError() << std::endl;
	}
}
