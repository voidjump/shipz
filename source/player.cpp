#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <SDL3/SDL.h>

#include "types.h"
#include "gfx.h"
#include "sound.h"
#include "other.h"
#include "player.h"
#include "team.h"
#include "base.h"
#include "assets.h"
#include "level.h"

const char * GetStatusString(int status) {
	switch(status) {
		case PLAYER_STATUS::DEAD:
			return "DEAD";
		case PLAYER_STATUS::FLYING:
			return "FLYING";
		case PLAYER_STATUS::LANDED:
			return "LANDED";
		case PLAYER_STATUS::JUSTCOLLIDEDROCK:
			return "JUSTCOLLIDEDROCK";
		case PLAYER_STATUS::JUSTSHOT:
			return "JUSTSHOT";
		case PLAYER_STATUS::RESPAWN:
			return "RESPAWN";
		case PLAYER_STATUS::LIFTOFF:
			return "LIFTOFF";
		case PLAYER_STATUS::SUICIDE:
			return "SUICIDE";
		case PLAYER_STATUS::JUSTCOLLIDEDBASE:
			return "JUSTCOLLIDEDBASE";
		case PLAYER_STATUS::LANDEDBASE :
			return "LANDEDBASE";
		case PLAYER_STATUS::LANDEDRESPAWN :
			return "LANDEDRESPAWN";
	}
	return "UNDEFINED";
}

void EmptyPlayer( Player * play )
{
	// empties a player array slot, so it's ready to accept a new player without problems.
	play->lastliftofftime = -3000;
	play->lastshottime = 0;
	play->bullet_shot = 0;
	play->bulletshotnr = 0;
	play->status = PLAYER_STATUS::DEAD;
	play->playing = 0;
	play->kills = 0;
	play->deaths = 0;
	play->Team = 0;
	strcpy( play->name , "            " );
	play->flamestate = 0;
	play->shipframe = 0;
	play->weapon = BULLET;
	play->x = 0;
	play->y = 0;
	play->vx = 0;
	play->vy = 0;
	play->fx = 0;
	play->fy = 0;
	play->angle = 0;
	play->engine_on = 0;
	play->y_bmp = 0;
	play->x_bmp = 0;
	play->crossx = 0;
	play->crossy = 0;
	play->self_sustaining = 0;
	play->playaddr = NULL;
	play->typing = 0;
}



inline int ConvertAngle( float angle )
{
	//converts an angle from float/game position ( 0 up ) to integer standard position ( 0 right )
	int temp_angle;
	temp_angle = int( angle ) + 90; // 0 degrees is right normall, we'll make it up.
	if( temp_angle > 359 )
		temp_angle -= 360;
	return temp_angle;
}



void InitPlayer( Player * play )
{
	// function is a little bit obsolete, should use emptyplayer instead. do this later!
	play->shipframe = 0;
	play->flamestate = 0;	
	play->x = 320;
	play->y = 290;
	play->angle = 0;
	play->kills = 0;
	play->deaths = 0;
	play->engine_on = 0;
	play->vx = 0;
	play->vy = 0;
	play->fx = 0;
	play->fy = 0;
	play->crossx = 0;
	play->crossy = -CROSSHAIRDIST;
	play->status = PLAYER_STATUS::DEAD;
	play->weapon = BULLET;
	play->typing = 0;
}

void UpdatePlayer( Player * play )
{
	// updates both local and non-local players. updates positions, stati, etc.
	// NOTE: this would be the perfect place for a recharge, like the laser battery ( ammo ) recharging
	if( !play->self_sustaining )
	{
		// this is the client himself ( a local player )
		if( play->status != PLAYER_STATUS::LANDED )
		{
			play->shipframe = ( int(play->angle) / 10 );
			if( play->shipframe > 35 ) 
			{
				play->shipframe = 35;
			}
			play->fy += float( GRAVITY * SHIPMASS );
			play->vy += float( play->fy ) * REALITYSCALE;
			play->vx += float( play->fx ) * REALITYSCALE;
			play->x += play->vx * ( deltatime / 1000 );
			play->y += play->vy * ( deltatime / 1000 );
		}
	}
	else
	{
		// this is a remote player
		play->x += play->vx * ( deltatime / 1000 );
		play->y += play->vy * ( deltatime / 1000 );
		
		play->crossy = look_sin[ConvertAngle( play->shipframe*10 )] * -CROSSHAIRDIST;
		play->crossx = look_cos[ConvertAngle( play->shipframe*10 )] * -CROSSHAIRDIST;	
	}
	if( play->engine_on )
	{
		if( play->flamestate == 1 ) { play->y_bmp = 30; }
		if( play->flamestate == 2 ) { play->y_bmp = 59; }
		if( play->flamestate == 3 ) { play->y_bmp = 88; }
		if( play->flamestate == 4 ) { play->y_bmp = 59; }
		if( play->flamestate == 5 ) { play->y_bmp = 30; }
	}
	else
	{
		play->y_bmp = 1;
	}
	play->x_bmp = 1 + (( play->shipframe) * ( 29 ));

	play->engine_on = 0;
	play->fx = 0;
	play->fy = 0;
}

void ResetPlayer( Player * play )
{
	// actually used for respawing a player
	// NOTE: should rename this function to the more appropriate 'RespawnPlayer'
	
	play->shipframe = 0;
	play->flamestate = 0;	
	play->x = 320;
	play->y = 290;
	play->angle = 0;
	play->vx = 0;
	play->vy = 0;
	play->fx = 0;
	play->fy = 0;
	play->crossx = 0;
	play->crossy = -CROSSHAIRDIST;
	play->weapon = BULLET;
}

void TestColmaps() {
	SDL_Surface *test_map;

	char tmpfilename[100];
	
	memset( tmpfilename, '\0', sizeof( tmpfilename ));
	snprintf( tmpfilename, 100, "%s./gfx/%s", SHAREPATH, "ship_collision.bmp" );
	
	test_map = SDL_LoadBMP(tmpfilename);
	test_map = SDL_ConvertSurface(test_map, SDL_PIXELFORMAT_RGBA8888);
	SDL_SaveBMP(test_map, "testoutput.bmp");
	
	std::cout << "testing colmaps" << std::endl;
	for( int x = 0; x < test_map->w; x++ )
	{
		for( int y = 0; y < test_map->h; y++ )
		{
			// std::cout << a << b;
			if (GetPixel(test_map, x, y) ){
				std::cout << "1";
			} else
			std::cout << "0";
		}
		std::cout << std::endl;
	}
}

void GetCollisionMaps( bool ** levelcolmap )
{
	// loads the levelcollisionmaps and shipcollisionmap from their image files.
	
	SDL_Surface *ship_collision;
	SDL_Surface *level_collision;

	ship_collision = LoadBMP("ship_collision.bmp");
	level_collision = LoadBMP(lvl.m_colmap_filename.c_str());
	
	for( int a = 0; a < lvl.m_width; a++ )
	{
		for( int b = 0; b < lvl.m_height; b++ )
		{
			levelcolmap[a][b] = GetPixel(level_collision, a, b);
		}
	}

	for( int a = 0; a < 36; a++ )
	{
		for( int b = 0; b < 28; b++ )
		{
			for( int c = 0; c < 28; c++ )
			{
				shipcolmap[a][b][c] = GetPixel(ship_collision, ((a*29)+1+b), c);
			}

		}
	}

	SDL_DestroySurface(level_collision);
	SDL_DestroySurface(ship_collision);
}

bool PlayerCollideWithLevel( Player * play, bool ** levelcolmap )
{
	// this function checks whether a players has crossed the level borders, or crashed with the level.
	// in both cases it returns 1.
	
	if( play->x > ( lvl.m_width - 14 ))
	{
		return true;
	}
	if( play->x < 14 )
	{
		return true;
	}
	if( play->y > ( lvl.m_height - 14 ))
	{
		return true;
	}
	if( play->y < 14 )
	{
		return true;
	}
	
	// fix this awful loop
	for( int a =0 ; a < 28 ; a++ )
	{
		for( int b = 0; b < 28; b++)
		{
			if( levelcolmap[int(play->x)+a-14][int(play->y)+b-14] && shipcolmap[play->shipframe][a][b] )
			{
				//player collided with level
				return true;
			}	
		}
	}
	//player didn't collide with level
	return false;
}

int PlayerCollideWithBullet( Player * play, int playernum, Player * players )
{
	// this function checks whether a player has collided with a bullet.
	// if so, it returns the bullet number. if not it returns -1.

	// NOTE: a better approach maybe would be to do the bullet tagging in this function, so when 2 bullets hit a ship
	// at the same time they both get erased. do this later (!)
	
	int cb = 0;
	for( cb = 0; cb < NUMBEROFBULLETS; cb++ )
	{
		if( players[ bullets[cb].owner - 1 ].Team != players[ playernum - 1 ].Team )
		{
			if( bullets[cb].type == ROCKET || bullets[cb].type == BULLET)
			{
				// first check if the bullet even is in the ships clipping rectangle, this saves time.
				if( bullets[cb].x > ( play->x - 1 ) && bullets[cb].x < ( play->x + 29 ) 
				&& bullets[cb].y > ( play->y - 1 ) && bullets[cb].y < ( play->y + 29 ))
				{
					// the bullet seems to be in the ships clipping rectangle,
					// check for pixel-precise collisions..
					// calculate the distance between bullet and ship so we can see how far
					// the bullet has entered the ships 'domain'
					int dx = int( bullets[cb].x - play->x );
					int dy = int( bullets[cb].y - play->y );
					if( dx > 0 && dx < 28 && dy > 0 && dy < 28 )
					{
						if( shipcolmap[ dx ][ dy ] )
						{
							return cb;	
						}
						if( dx < 27 )	// if we don't do this might get a segsev
						{
							if( shipcolmap[play->shipframe][ dx + 1 ][ dy ] )
							{
								// return the bullet number, so the main function can use
								// it to tag the bullet etc. same for the next 2 returns.
								return cb;
							}
						}
						if( dy < 27 )	// same here..
						{
							if( shipcolmap[play->shipframe][ dx ][ dy + 1 ] )
							{
								return cb;
							}
						}
						if( dy < 27 && dx < 27 ) // and here..
						{
							if( shipcolmap[play->shipframe][ dx + 1 ][ dy + 1 ] )
							{
								return cb;
							}
						}
					}
				}
			}
		}
		if( bullets[cb].type == MINE )
		{
			int dx = int( bullets[cb].x - play->x );
			int dy = int( bullets[cb].y - play->y );
			if( sqrt( dx*dx + dy*dy ) < MINEDETONATERADIUS && 
					SDL_GetTicks() - bullets[cb].minelaidtime > MINEACTIVATIONTIME )
			{
				return cb;
			}
		}
	}
	// if this point is reached, no bullet has collided with the players' ship, so apparently he is safe..
	// FOR NOW!!! NEXT TIME GADGET! NEXT TIME!!!!!!!!!!
	return -1; // return no bullet collide signal.
}

int PlayerCollideWithBase( Player * play )
{
	// returns the number of the Base that is touching, if no Base is touching, returns -1
	int i;
	for( i = 0; i < lvl.m_num_bases; i++ )
	{
		if( int( play->y ) < bases[i].y-33 ||
		    int( play->y ) > bases[i].y+17 ||
		    int( play->x ) < bases[i].x-37 ||
		    int( play->x ) > bases[i].x+37 )
		{
			// player isn't touching Base for sure
		}
		else
		{
			// there is a chance player is touching Base
			for( int a =0 ; a < 28 ; a++ )
			{
				for( int b = 0; b < 28; b++)
				{
					if( int(play->x)+a-14 > bases[i].x - 21 && int(play->x)+a-14 < bases[i].x + 22 &&
					    int(play->y)+b-14 > bases[i].y - 18 && int(play->y)+b-14 < bases[i].y - 7 && 
						shipcolmap[play->shipframe][a][b] )
					{
						//player collided with level
						return i; // return the number of the Base.
					}	
				}
			}
		}
		
	}
	// for comment see previous function
	return -1;
}

void AdjustViewport( Player * play )
{
	// this function focusses the viewport on a player. usually this is the normal player, but it can also be used
	// for spectator view or things like that
	int tempx = int(play->x), tempy = int(play->y);
	viewportx = tempx - 320;
	viewporty = tempy - 240;
	if( viewportx < 0 ) { viewportx = 0; }
	if( viewporty < 0 ) { viewporty = 0; }
	if( viewportx > ( lvl.m_width - XRES - 1 )) { viewportx = lvl.m_width - XRES - 1; }
	if( viewporty > ( lvl.m_height - YRES - 1 )) { viewporty = lvl.m_height - YRES - 1; }
}

void PlayerRot( Player * play, bool clockwise )
{	
	// rotates a player, scaled by the deltatime interval.
	// the use of clockwise is pretty obvious i think..
	if( clockwise )
	{
		play->angle += float( ROTATIONSPEED * ( deltatime / 1000 ));
		if( play->angle > 359 )
		{
			play->angle -= 360;
		}
		play->crossy = look_sin[ConvertAngle( play->angle )] * -CROSSHAIRDIST;
		play->crossx = look_cos[ConvertAngle( play->angle )] * -CROSSHAIRDIST;	
	}
	else
	{
		play->angle -= float( ROTATIONSPEED * ( deltatime / 1000 ));
		if( play->angle < 0 )
		{
			play->angle += 360;
		}
		play->crossy = look_sin[ConvertAngle( play->angle )] * -CROSSHAIRDIST;
		play->crossx = look_cos[ConvertAngle( play->angle )] * -CROSSHAIRDIST;
	}
}
void PlayerThrust( Player * play )
{
	// thrusts a player forward, scaled by deltatime interval
	play->engine_on = 1;
	play->flamestate++;
	if( play->flamestate > 6 )
	{ play->flamestate = 1; }
	play->fy -= look_sin[ConvertAngle( play->angle )] * THRUST;
	play->fx -= look_cos[ConvertAngle( play->angle )] * THRUST;

}
Uint16 ShootBullet( Player * play, int owner )
{
	// searches for an empty spot in the bullets array and adds a bullet there, then returns the array index.
	// note that each player has an own section in an array, so two players will never 'allocate' the same bullet
	// slot.
	for( Uint16 search = (owner-1)*(NUMBEROFBULLETS/8); search < owner*(NUMBEROFBULLETS/8); search++ )
	{
		if( bullets[search].active == 0 )
		{
			bullets[search].active = 1;
			bullets[search].x = play->x;
			bullets[search].y = play->y;
			bullets[search].type = play->weapon;
			if( play->weapon == BULLET )
			{
				bullets[search].vx = look_cos[ConvertAngle( play->angle )] * BULLETSPEED;
				bullets[search].vy = look_sin[ConvertAngle( play->angle )] * BULLETSPEED;

			}
			if( play->weapon == ROCKET )
			{
				bullets[search].vx = look_cos[ConvertAngle( play->angle )] * BULLETSPEED;
				bullets[search].vy = look_sin[ConvertAngle( play->angle )] * BULLETSPEED;
				bullets[search].angle = play->angle;
				PlaySound( rocketsound );
			}
			if( play->weapon == MINE )
			{
				bullets[search].minelaidtime = SDL_GetTicks();
			}
			bullets[search].owner = owner;
			bullets[search].collide = 0;
			return search;
		}
	}
	return 6666;
}

int GetNearestEnemyPlayer( Player * plyrs, int x, int y, int pteam )
{
	// returns a number to the enemy player nearest to x,y
	int i; // loop thingy
	int tdx, tdy; // temp dist
	float dist = 1000000; // dist to nearest player, arbitrarily set very high.
	float tdist; // temp value for calc of dist.
	int plr=-2; // the nearest player so far

	for( i = 0; i < MAXPLAYERS; i++ )
	{
		if( plyrs[i].playing && plyrs[i].status == PLAYER_STATUS::FLYING  )
		{
			tdx = int(x - plyrs[i].x);
			tdy = int(y - plyrs[i].y);
			
			tdist = sqrt(( tdx * tdx ) + ( tdy * tdy ));
			if( tdist < dist && plyrs[i].Team != pteam )
			{
				dist = tdist;
				plr = i;
			}
			
		}
	}
	return plr+1;

}

void UpdateBullets( Player * plyrs)
{
	// updates the positions of all active bullets, scaled by deltatime interval

	for( int upd = 0; upd < NUMBEROFBULLETS; upd++ )
	{
		if( bullets[upd].active == 1 )
		{
			if( bullets[upd].type == BULLET )
			{
				bullets[upd].x -= bullets[upd].vx * (deltatime/1000);
				bullets[upd].y -= bullets[upd].vy * (deltatime/1000);
				continue;
			}
			if( bullets[upd].type == ROCKET )
			{
				int nearest = 0;
				nearest = GetNearestEnemyPlayer( plyrs, int(bullets[upd].x),
						int(bullets[upd].y), bullets[upd].owner );
				if ( nearest != -1 )	// if no enemyplayer is found don't steer
				{
					int xd = int(bullets[upd].x - plyrs[nearest-1].x);
					int yd = int(bullets[upd].y - plyrs[nearest-1].y);
					float dis, dirz, Tx, Ty, Rx, Ry, CrossProd;
						
					dis = (float) sqrt(xd * xd + yd * yd);
					if( !( dis > ROCKETRADARRADIUS )) // can the rocket see the player??
					{
						// target vector
						Tx = xd/dis;
						Ty = yd/dis;
						// rocket direction vector
						Rx = look_cos[ConvertAngle( bullets[upd].angle )];
						Ry = look_sin[ConvertAngle( bullets[upd].angle )];
						// calculate the cross product
						CrossProd = ( Tx * Ry - Rx * Ty );
						if( CrossProd == 0 )
						{
							// the rocket is turned 180 degrees away from target
							// should really do a random turn here but what the heck:
							bullets[upd].angle += MAXROCKETTURN * (deltatime/1000);
							if( bullets[upd].angle > 359 )
							{
								bullets[upd].angle -= 360;
							}
						}
						if( CrossProd > 0 )
						{
							// rotate counterclockwise
							bullets[upd].angle -= MAXROCKETTURN * (deltatime/1000);
							if( bullets[upd].angle < 0 )
							{
								bullets[upd].angle += 359;
							}
						}
						if( CrossProd < 0 )
						{
							// rotate clockwise
							bullets[upd].angle += MAXROCKETTURN * (deltatime/1000);
							if( bullets[upd].angle > 360 )
							{
								bullets[upd].angle -= 359;
							}
						}
					}
				}
				// update rocket coordinates
				bullets[upd].x -=
					look_cos[ConvertAngle( bullets[upd].angle )] * ROCKETSPEED * (deltatime/1000);
				bullets[upd].y -= 
					look_sin[ConvertAngle( bullets[upd].angle )] * ROCKETSPEED * (deltatime/1000);
			}
		}
	}
}

void CheckBulletCollides( bool ** colmap )
{
	// checks if any active bullets have collided with the level, and, if they have, tags them.
	int i;
	for( i = 0; i < NUMBEROFBULLETS; i++ )
	{
		if( bullets[i].active == 1 )
		{
			if( bullets[i].type == BULLET || bullets[i].type == ROCKET )
			{
				if( bullets[i].x > (lvl.m_width - 1) || bullets[i].x < 0 )
				{
					bullets[i].collide = 1;
					continue;
				}
				if( bullets[i].y > (lvl.m_height - 1) || bullets[i]. y < 0 )
				{
					bullets[i].collide = 1;
					continue;
				}
				if( colmap[ int( bullets[i].x ) ][ int( bullets[i].y ) ] == 1 )
				{
					bullets[i].collide = 1;
					continue;
				}
				if( colmap[ int( bullets[i].x ) + 1 ][ int( bullets[i].y ) ] == 1 )
				{
					bullets[i].collide = 1;
					continue;
				}
				if( colmap[ int( bullets[i].x ) ][ int( bullets[i].y ) + 1 ] == 1 )
				{
					bullets[i].collide = 1;
					continue;
				}
				if( colmap[ int( bullets[i].x ) + 1 ][ int( bullets[i].y ) + 1 ] == 1 )
				{
					bullets[i].collide = 1;
					continue;
				}
			}
			if( bullets[i].type == MINE )
			{
				if( SDL_GetTicks() - bullets[i].minelaidtime > MINELIFETIME )
				{
					bullets[i].collide = 1;
				}
			}
		}
	}
}

int FindRespawnBase( int rspwnteam )
{
	int i, rndint, tmpctr=0;
	srand(SDL_GetTicks());
	if( rspwnteam == BLUE )
	{
		rndint = int(rand())%int(blue_team.bases) + 1;
		for( i = 0; i < MAXBASES; i++ )
		{
			if( bases[i].used )
			{
				if( bases[i].owner == BLUE )
				{	
					tmpctr++;
					if( tmpctr == rndint )
					{
						return i;
					}
				}
			}
		}
	}
	if( rspwnteam == SHIPZ_TEAM::RED )
	{
		rndint = int(rand())%int(red_team.bases) + 1;
		for( i = 0; i < MAXBASES; i++ )
		{
			if( bases[i].used )
			{
				if( bases[i].owner == SHIPZ_TEAM::RED )
				{	
					tmpctr++;
					if( tmpctr == rndint )
					{
						return i;
					}
				}
			}
		}
			
	}
	// if the function arrives here, something is definately wrong...
	return -1;
}

void CleanBullet( int num )
{
	// cleans up a bullet, so it can be used again
	bullets[num].active = 0;
	bullets[num].owner = 0;
	bullets[num].collide = 0;
	bullets[num].type = 0;
	bullets[num].x = 0;
	bullets[num].y = 0;
	bullets[num].vx = 0;
	bullets[num].vy = 0;
	bullets[num].angle = 0;
	bullets[num].minelaidtime = 0;
}
