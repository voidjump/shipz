#include <math.h>
#include <iostream>
#include <fstream>
#include <string.h>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL_keycode.h>

#include "types.h"
#include "player.h"
#include "sound.h"

// Load an image from the /gfx/ directory
// Returns surface containing loaded image
// TODO: Handle load failure?
SDL_Surface * LoadIMG( const char * filename )
{
	// Append filename to path
	char tmppath[256];
	memset( tmppath, '\0', sizeof( tmppath ));
	snprintf( tmppath, 256, "%s./gfx/%s", SHAREPATH, filename );

	SDL_Surface * loaded_image;
	loaded_image = IMG_Load(tmppath);
	if( loaded_image == NULL ) {
		std::cout << "Could not load image " << filename << std::endl;
		SDL_Quit();
	}
	return loaded_image;
}

void CreateGonLookup() // create lookup tables for sin and cos
{
	int i = 0;
	for( i = 0; i <= 359; i++ )
	{
		double theta = double(i * ( PI / 180 )); // convert the values
		look_sin[i] = float( sin( theta ));
		look_cos[i] = float( cos( theta ));
	}
	look_cos[90] = 0; // somehow these values aren't converted correctly so we have to input them manually
	look_sin[180] = 0;
	look_cos[270] = 0;
}

void InitSDL()
{
	if ( !SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO))
	{
		std::cout << "Unable to init SDL:" << std::endl << SDL_GetError();
		exit(1);
	}
	// always do SDL_Quit, whatever happens.
	atexit( SDL_Quit );
}

bool LoadLevelFile()
{
	// searches the levelcodes file for a match. if found
	// proceeds to check the levellist file for level availability
	// then loads the levelname & dimensions into the LevelData lvl structure.
	// if found, returns 1, if not, returns 0
	bool READFLAG = 1, succes = 0;
	int i = 0;
	char * tmppoint, tempread[80], trash;
	char tmpfilename[100];
	int mypos;
	int bascur; // the current basetag we're dealing with
	int basenr = 0;
	

	memset( tmpfilename, '\0', sizeof(tmpfilename) );

	snprintf( tmpfilename, 100, "%s./levels/%s", SHAREPATH, lvl.filename );
	
	std::ifstream invoer( tmpfilename );
	if( invoer )
	{
		succes = 1;
		
		std::cout << std::endl << "@ reading levelfile: " << lvl.filename << std::endl;
		
		while( READFLAG )
		{
			invoer.getline( tempread, 100 );
			tmppoint = tempread;
			
			if( strstr( tempread, "end" ))
			{
				tmppoint+=4;
				break;
			}
			if( strstr( tempread, "author=" ))
			{
				tmppoint+=7;
				std::cout << "  author: " << tmppoint << std::endl;
				strcpy( lvl.author, tmppoint );
				continue;
			}
			if( strstr( tempread, "name=" ))
			{
				tmppoint+=5;
				std::cout << "  name: " << tmppoint << std::endl;
				strcpy( lvl.name, tmppoint );
				continue;
			}
			if( strstr( tempread, "version=" ))
			{
				tmppoint+=8;
				std::cout << "  version: " << tmppoint << std::endl;
				lvl.levelversion = atoi( tmppoint );
				continue;
			}
			if( strstr( tempread, "bases=" ))
			{
				tmppoint+=6;
				std::cout << "  bases: " << tmppoint << std::endl;
				lvl.bases = atoi( tmppoint );
				continue;
			}
			if( strstr( tempread, "collisionmap=" ))
			{
				tmppoint+=13;
				std::cout << "  collisionmap: " << tmppoint << std::endl;
				strcpy( lvl.colmap, tmppoint );
				continue;
			}
			if( strstr( tempread, "image=" ))
			{
				tmppoint+=6;
				std::cout << "  image: " << tmppoint << std::endl;
				strcpy( lvl.image, tmppoint );
				continue;
			}
			if( strstr( tempread, "basesblue=" ))
			{
				tmppoint+=10;
				std::cout << "  blue bases: " << tmppoint << std::endl;
				bascur = BLUE;
				continue;
			}
			if( strstr( tempread, "basesred=" ))
			{
				tmppoint+=9;
				std::cout << "  red bases: " << tmppoint << std::endl;
				bascur = RED;
				continue;
			}
			if( strstr( tempread, "basesneutral=" ))
			{
				tmppoint+=13;
				std::cout << "  neutral bases: " << tmppoint << std::endl;
				bascur = NEUTRAL;
				continue;
			}
			if( strstr( tempread, "basex=" ))
			{
				tmppoint+=6;
				bases[basenr].x = atoi( tmppoint );
				continue;
			}
			if( strstr( tempread, "basey=" ))
			{
				tmppoint+=6;
				if( bascur == RED )
				{
					red_team.bases++;
				}
				if( bascur == BLUE )
				{
					blue_team.bases++;
				}
				bases[basenr].used = 1;
				bases[basenr].owner = bascur;
				bases[basenr].y = atoi( tmppoint );
				basenr++;
				continue;
			}

			if( invoer.eof() )
			{
				READFLAG = 1;
			}
		}
		SDL_Surface *level = LoadIMG( lvl.image );
		lvl.width = level->w;
		lvl.height = level->h;
		SDL_DestroySurface(level);
		std::cout << "  size: " << lvl.width << " x " << lvl.height << std::endl;
		std::cout << "@ done reading." << std::endl;
	}
	invoer.close();
	if ( !succes )
	{
		std::cout << tmpfilename << std::endl;
	}
	return succes;
}

int GetNearestBase( int x, int y )
{
	// returns a number to the Base nearest to x,y
	int i; // loop thingy
	int tdx, tdy; // temp dist
	float dist, tdist; // the dist of the nearest Base so far, temp value for calc of dist.
	int Base; // the nearest Base so far

	for( i = 0; i < lvl.bases; i++ )
	{
		tdx = x - bases[i].x;
		tdy = y - bases[i].y;
		
		tdist =sqrt(( tdx * tdx ) + ( tdy * tdy ));
		if( tdist < dist || i == 0 )
		{
			dist = tdist;
			Base = i;
		}
	}
	return Base;
}

void NewExplosion( int x, int y )
{
	int i;
	for( i = 0; i < NUMBEROFEXPLOSIONS; i++ )
	{
		if( explosions[i].used == 0 )
		{
			explosions[i].used = 1;
			explosions[i].x = x;
			explosions[i].y = y;
			explosions[i].starttime = SDL_GetTicks();
			break;
		}
	}
	PlaySound( explodesound );
}

void ClearOldExplosions()
{
	int i;
	for( i = 0; i < NUMBEROFEXPLOSIONS; i++ )
	{
		if( explosions[i].used == 1 )
		{	
			float dt = SDL_GetTicks() - explosions[i].starttime;
			if( dt > EXPLOSIONLIFE )
			{
				explosions[i].used = 0;
				explosions[i].x = 0;
				explosions[i].y = 0;
				explosions[i].starttime = 0;
			}
			else
			{
				explosions[i].frame = int(dt) / EXPLOSIONFRAMETIME;
				if( explosions[i].frame == 10 ) 
				{
					explosions[i].frame = 9;
				}
			}
		}
	}

}

void CleanAllExplosions()
{
	int i;
	for( i = 0; i < NUMBEROFEXPLOSIONS; i++ )
	{
		explosions[i].used = 0;
		explosions[i].x = 0;
		explosions[i].y = 0;
		explosions[i].starttime = 0;
	}
}

bool CheckForQuitSignal()
{
	SDL_Event event;
	while ( SDL_PollEvent(&event) )
	{
		if ( event.type == SDL_EVENT_QUIT )  {  return 1;  }
		if ( event.type == SDL_EVENT_KEY_DOWN )
		{
			if ( event.key.key == SDLK_ESCAPE )
			{
				return 1;
			}
		}
	}
	return 0;
}	



void GetTyping( char * buffer, uint key, uint mod, int * charstyped )
{
	bool shift = 0; bool caps = 0;
	if( mod & SDL_KMOD_LSHIFT || mod & SDL_KMOD_RSHIFT ) { shift = 1; }
	if( mod & SDL_KMOD_CAPS ) { caps = 1; }

	if( charstyped[0] < 79 )
	{	
		// a-z / A-Z
		if( (shift && !caps) || caps )
		{
			if( key == SDLK_A  ) { buffer[ charstyped[0] ] = 'A'; charstyped[0]++; } 
			if( key == SDLK_B  ) { buffer[ charstyped[0] ] = 'B'; charstyped[0]++; }
			if( key == SDLK_C  ) { buffer[ charstyped[0] ] = 'C'; charstyped[0]++; }
			if( key == SDLK_D  ) { buffer[ charstyped[0] ] = 'D'; charstyped[0]++; }
			if( key == SDLK_E  ) { buffer[ charstyped[0] ] = 'E'; charstyped[0]++; }
			if( key == SDLK_F  ) { buffer[ charstyped[0] ] = 'F'; charstyped[0]++; }
			if( key == SDLK_G  ) { buffer[ charstyped[0] ] = 'G'; charstyped[0]++; }
			if( key == SDLK_H  ) { buffer[ charstyped[0] ] = 'H'; charstyped[0]++; } 
			if( key == SDLK_I  ) { buffer[ charstyped[0] ] = 'I'; charstyped[0]++; }
			if( key == SDLK_J  ) { buffer[ charstyped[0] ] = 'J'; charstyped[0]++; }
			if( key == SDLK_K  ) { buffer[ charstyped[0] ] = 'K'; charstyped[0]++; }
			if( key == SDLK_L  ) { buffer[ charstyped[0] ] = 'L'; charstyped[0]++; }
			if( key == SDLK_M  ) { buffer[ charstyped[0] ] = 'M'; charstyped[0]++; }
			if( key == SDLK_N  ) { buffer[ charstyped[0] ] = 'N'; charstyped[0]++; }
			if( key == SDLK_O  ) { buffer[ charstyped[0] ] = 'O'; charstyped[0]++; } 
			if( key == SDLK_P  ) { buffer[ charstyped[0] ] = 'P'; charstyped[0]++; }
			if( key == SDLK_Q  ) { buffer[ charstyped[0] ] = 'Q'; charstyped[0]++; }
			if( key == SDLK_R  ) { buffer[ charstyped[0] ] = 'R'; charstyped[0]++; }
			if( key == SDLK_S  ) { buffer[ charstyped[0] ] = 'S'; charstyped[0]++; }
			if( key == SDLK_T  ) { buffer[ charstyped[0] ] = 'T'; charstyped[0]++; }
			if( key == SDLK_U  ) { buffer[ charstyped[0] ] = 'U'; charstyped[0]++; }
			if( key == SDLK_V  ) { buffer[ charstyped[0] ] = 'V'; charstyped[0]++; } 
			if( key == SDLK_W  ) { buffer[ charstyped[0] ] = 'W'; charstyped[0]++; }
			if( key == SDLK_X  ) { buffer[ charstyped[0] ] = 'X'; charstyped[0]++; }
			if( key == SDLK_Y  ) { buffer[ charstyped[0] ] = 'Y'; charstyped[0]++; }
			if( key == SDLK_Z  ) { buffer[ charstyped[0] ] = 'Z'; charstyped[0]++; }
		}	
		else
		{
			if( key == SDLK_A  ) { buffer[ charstyped[0] ] = 'a'; charstyped[0]++; } 
			if( key == SDLK_B  ) { buffer[ charstyped[0] ] = 'b'; charstyped[0]++; }
			if( key == SDLK_C  ) { buffer[ charstyped[0] ] = 'c'; charstyped[0]++; }
			if( key == SDLK_D  ) { buffer[ charstyped[0] ] = 'd'; charstyped[0]++; }
			if( key == SDLK_E  ) { buffer[ charstyped[0] ] = 'e'; charstyped[0]++; }
			if( key == SDLK_F  ) { buffer[ charstyped[0] ] = 'f'; charstyped[0]++; }
			if( key == SDLK_G  ) { buffer[ charstyped[0] ] = 'g'; charstyped[0]++; }
			if( key == SDLK_H  ) { buffer[ charstyped[0] ] = 'h'; charstyped[0]++; } 
			if( key == SDLK_I  ) { buffer[ charstyped[0] ] = 'i'; charstyped[0]++; }
			if( key == SDLK_J  ) { buffer[ charstyped[0] ] = 'j'; charstyped[0]++; }
			if( key == SDLK_K  ) { buffer[ charstyped[0] ] = 'k'; charstyped[0]++; }
			if( key == SDLK_L  ) { buffer[ charstyped[0] ] = 'l'; charstyped[0]++; }
			if( key == SDLK_M  ) { buffer[ charstyped[0] ] = 'm'; charstyped[0]++; }
			if( key == SDLK_N  ) { buffer[ charstyped[0] ] = 'n'; charstyped[0]++; }
			if( key == SDLK_O  ) { buffer[ charstyped[0] ] = 'o'; charstyped[0]++; } 
			if( key == SDLK_P  ) { buffer[ charstyped[0] ] = 'p'; charstyped[0]++; }
			if( key == SDLK_Q  ) { buffer[ charstyped[0] ] = 'q'; charstyped[0]++; }
			if( key == SDLK_R  ) { buffer[ charstyped[0] ] = 'r'; charstyped[0]++; }
			if( key == SDLK_S  ) { buffer[ charstyped[0] ] = 's'; charstyped[0]++; }
			if( key == SDLK_T  ) { buffer[ charstyped[0] ] = 't'; charstyped[0]++; }
			if( key == SDLK_U  ) { buffer[ charstyped[0] ] = 'u'; charstyped[0]++; }
			if( key == SDLK_V  ) { buffer[ charstyped[0] ] = 'v'; charstyped[0]++; } 
			if( key == SDLK_W  ) { buffer[ charstyped[0] ] = 'w'; charstyped[0]++; }
			if( key == SDLK_X  ) { buffer[ charstyped[0] ] = 'x'; charstyped[0]++; }
			if( key == SDLK_Y  ) { buffer[ charstyped[0] ] = 'y'; charstyped[0]++; }
			if( key == SDLK_Z  ) { buffer[ charstyped[0] ] = 'z'; charstyped[0]++; }
		}
						
		
		
		// end a-z / A-Z
		if( shift )
		{
			if( key == SDLK_PERIOD ) { buffer[ charstyped[0] ] = '>'; charstyped[0]++; }
			if( key == SDLK_COMMA ) { buffer[ charstyped[0] ] = '<'; charstyped[0]++; }
			if( key == SDLK_SEMICOLON ) { buffer[ charstyped[0] ] = ':'; charstyped[0]++; }
			if( key == SDLK_SPACE  ) { buffer[ charstyped[0] ] = ' '; charstyped[0]++; }
			if( key == SDLK_MINUS ) { buffer[ charstyped[0] ] = '_'; charstyped[0]++; }
			if( key == SDLK_APOSTROPHE ) { buffer[ charstyped[0] ] = '\"'; charstyped[0]++; }
			if( key == SDLK_SLASH ) { buffer[ charstyped[0] ] = '?'; charstyped[0]++; }

			if( key == SDLK_0  ) { buffer[ charstyped[0] ] = ')'; charstyped[0]++; }
			if( key == SDLK_1  ) { buffer[ charstyped[0] ] = '!'; charstyped[0]++; } 
			if( key == SDLK_2  ) { buffer[ charstyped[0] ] = '@'; charstyped[0]++; }
			if( key == SDLK_3  ) { buffer[ charstyped[0] ] = '#'; charstyped[0]++; }
			if( key == SDLK_4  ) { buffer[ charstyped[0] ] = '$'; charstyped[0]++; }
			if( key == SDLK_5  ) { buffer[ charstyped[0] ] = '%'; charstyped[0]++; }
			if( key == SDLK_6  ) { buffer[ charstyped[0] ] = '^'; charstyped[0]++; }
			if( key == SDLK_7  ) { buffer[ charstyped[0] ] = '&'; charstyped[0]++; }
			if( key == SDLK_8  ) { buffer[ charstyped[0] ] = '*'; charstyped[0]++; } 
			if( key == SDLK_9  ) { buffer[ charstyped[0] ] = '('; charstyped[0]++; }
		}
		else
		{
			if( key == SDLK_PERIOD ) { buffer[ charstyped[0] ] = '.'; charstyped[0]++; }
			if( key == SDLK_COMMA ) { buffer[ charstyped[0] ] = ','; charstyped[0]++; }
			if( key == SDLK_SEMICOLON ) { buffer[ charstyped[0] ] = ';'; charstyped[0]++; }
			if( key == SDLK_SPACE  ) { buffer[ charstyped[0] ] = ' '; charstyped[0]++; }
			if( key == SDLK_MINUS ) { buffer[ charstyped[0] ] = '-'; charstyped[0]++; }
			if( key == SDLK_APOSTROPHE ) { buffer[ charstyped[0] ] = '\''; charstyped[0]++; }
			if( key == SDLK_SLASH ) { buffer[ charstyped[0] ] = '/'; charstyped[0]++; }

			
			if( key == SDLK_0  ) { buffer[ charstyped[0] ] = '0'; charstyped[0]++; } 
			if( key == SDLK_1  ) { buffer[ charstyped[0] ] = '1'; charstyped[0]++; }
			if( key == SDLK_2  ) { buffer[ charstyped[0] ] = '2'; charstyped[0]++; }
			if( key == SDLK_3  ) { buffer[ charstyped[0] ] = '3'; charstyped[0]++; }
			if( key == SDLK_4  ) { buffer[ charstyped[0] ] = '4'; charstyped[0]++; }
			if( key == SDLK_5  ) { buffer[ charstyped[0] ] = '5'; charstyped[0]++; }
			if( key == SDLK_6  ) { buffer[ charstyped[0] ] = '6'; charstyped[0]++; }
			if( key == SDLK_7  ) { buffer[ charstyped[0] ] = '7'; charstyped[0]++; } 
			if( key == SDLK_8  ) { buffer[ charstyped[0] ] = '8'; charstyped[0]++; }
			if( key == SDLK_9  ) { buffer[ charstyped[0] ] = '9'; charstyped[0]++; }
		}
	}
	if( charstyped[0] > 0 )
	{
		if( key == SDLK_BACKSPACE  )
		{ buffer[ charstyped[0] - 1 ] = 0; charstyped[0]--; }
	}

}

void EndMessage()
{
	std::cout << std::endl << std::endl;
	std::cout << "Thank you for playing shipz." << std::endl;
	std::cout << "Game code and graphics by Steve van Bennekom." << std::endl;
	std::cout << "Scripts, makefiles and code maintainance by Raymond Jelierse" << std::endl;
	std::cout << std::endl;
}

// Attempt to load a .wav file from /sound/ dir and return a Mix_Chunk 
Mix_Chunk * LoadSound ( const char * filename )
{
	Mix_Chunk * loaded_chunk;

	// Append .wav file to /sound/ dir
	char tempstr[200];
	memset( tempstr, '\0', sizeof( tempstr ));
	snprintf( tempstr, 200, "%s/sound/%s", SHAREPATH, filename );

	loaded_chunk = Mix_LoadWAV(tempstr);
	if(!loaded_chunk)
	{
	    std::cout << "Mix_LoadWAV: " <<  SDL_GetError() << std::endl;
	    return NULL;
	}
	return loaded_chunk;
}
