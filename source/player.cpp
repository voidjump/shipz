#include "player.h"

#include <SDL3/SDL.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <limits>

#include "assets.h"
#include "base.h"
#include "gfx.h"
#include "level.h"
#include "log.h"
#include "other.h"
#include "session.h"
#include "sound.h"
#include "team.h"
#include "types.h"

// all instances
std::map<ClientID, Player *> Player::instances;

// Allocate new clientID
ClientID Player::GeneratePlayerID() {
    for (ClientID id = 1; id < std::numeric_limits<ClientID>::max(); id++) {
        if (instances.find(id) == instances.end()) {
            return id;
        }
    }
    throw new std::runtime_error("no free player ID slots available");
    return 0;
}

Player::Player(ShipzSession *session) {
    this->session = session;
    this->player_id = GeneratePlayerID();
    instances[this->player_id] = this;
    this->Init();
}

Player::Player(uint16_t id) {
    this->session = nullptr;
    this->player_id = id;
    instances[this->player_id] = this;
    this->Init();
}

Player::~Player() { instances.erase(this->player_id); }

// Retrieve a player instance by their ID
Player *Player::GetByID(uint16_t search_id) {
    if (instances.find(search_id) != instances.end()) {
        return instances[search_id];
    }
    return nullptr;
}

// Is the player currently landed?
bool Player::IsLanded() { return this->status == PLAYER_STATUS::LANDED; }

// Is the player alive
bool Player::IsAlive() { return this->status != PLAYER_STATUS::DEAD; }

// Is the player flying
bool Player::IsFlying() { return this->status != PLAYER_STATUS::FLYING; }

// Lift Off a player, return true if succesful
// Serverside
// TODO: Add serverside assertion
bool Player::LiftOff() {
    if (this->IsLanded()) {
        log::info("player ", this->name, " is lifting off");
        this->status = PLAYER_STATUS::FLYING;
        this->y -= 10;
        return true;
    }
    return false;
}

// Spawn a player, return true if succesful
// Serverside
// TODO: Add serverside assertion
bool Player::Spawn() {
    if (!this->IsAlive()) {
        log::info("player ", this->name, " is respawning");
        this->status = PLAYER_STATUS::LANDED;
        return true;
    }
    return false;
}

void Player::HandleUpdate(SyncPlayerState *sync) {
    uint16_t temp_status = sync->status_bits;
    if (this->is_local) {
        this->status = sync->status_bits;
        this->typing = sync->typing;
        this->x = sync->x;
        this->y = sync->y;
        this->vx = sync->vx;
        this->vy = sync->vy;
    } else {
        this->status = sync->status_bits;
    }
}

const char *GetStatusString(int status) {
    // switch(status) {
    // 	case PLAYER_STATUS::DEAD:
    // 		return "DEAD";
    // 	case PLAYER_STATUS::FLYING:
    // 		return "FLYING";
    // 	case PLAYER_STATUS::LANDED:
    // 		return "LANDED";
    // 	case PLAYER_STATUS::JUSTCOLLIDEDROCK:
    // 		return "JUSTCOLLIDEDROCK";
    // 	case PLAYER_STATUS::JUSTSHOT:
    // 		return "JUSTSHOT";
    // 	case PLAYER_STATUS::RESPAWN:
    // 		return "RESPAWN";
    // 	case PLAYER_STATUS::LIFTOFF:
    // 		return "LIFTOFF";
    // 	case PLAYER_STATUS::SUICIDE:
    // 		return "SUICIDE";
    // 	case PLAYER_STATUS::JUSTCOLLIDEDBASE:
    // 		return "JUSTCOLLIDEDBASE";
    // 	case PLAYER_STATUS::LANDEDBASE :
    // 		return "LANDEDBASE";
    // 	case PLAYER_STATUS::LANDEDRESPAWN :
    // 		return "LANDEDRESPAWN";
    // }
    return "UNDEFINED";
}

// Initialize fields on a player
void Player::Init() {
    this->shipframe = 0;
    this->flamestate = 0;
    this->angle = 0;
    this->kills = 0;
    this->deaths = 0;
    this->engine_on = 0;
    this->x = 320;
    this->y = 240;
    this->vx = 0;
    this->vy = 0;
    this->fx = 0;
    this->fy = 0;
    this->crossx = 0;
    this->crossy = -CROSSHAIRDIST;
    this->status = PLAYER_STATUS::DEAD;
    this->weapon = WEAPON_BULLET;
    this->typing = false;
    this->lastliftofftime = -LIFTOFFSHOOTDELAY;
    this->lastshottime = 0;
    this->team = Player::GetBalancedTeam();
    this->name.clear();
    this->y_bmp = 0;
    this->x_bmp = 0;
    this->is_local = 0;
}

TeamID Player::GetBalancedTeam() {
    if (Player::GetTeamCount(SHIPZ_TEAM::RED) <=
        Player::GetTeamCount(SHIPZ_TEAM::BLUE)) {
        return SHIPZ_TEAM::RED;
    } else {
        return SHIPZ_TEAM::BLUE;
    }
}

void Player::Update(float delta) {
    // updates both local and non-local players. updates positions, stati, etc.
    if (!this->is_local) {
        if (this->status != PLAYER_STATUS::LANDED) {
            this->shipframe = (int(this->angle) / 10);
            if (this->shipframe > 35) {
                this->shipframe = 35;
            }
            // Add trust based on the angle
            if(this->engine_on) {
                this->Thrust();
            }
            this->vy = this->vy + (this->fy * delta);
            this->vx = this->vx + (this->fx * delta);

            this->vy = this->vy + (float(GRAVITY) * delta);
            this->x += this->vx * delta;
            this->y += this->vy * delta;
        }
    } else {
        // this is a remote player
        this->x += this->vx * (delta / 1000);
        this->y += this->vy * (delta / 1000);

        this->crossy =
            look_sin[ConvertAngle(this->angle)] * -CROSSHAIRDIST;
        this->crossx =
            look_cos[ConvertAngle(this->angle)] * -CROSSHAIRDIST;
    }
    if (this->engine_on) {
        this->flamestate = (this->flamestate + 1) % 6;
        if (this->flamestate == 1) {
            this->y_bmp = 30;
        }
        if (this->flamestate == 2) {
            this->y_bmp = 59;
        }
        if (this->flamestate == 3) {
            this->y_bmp = 88;
        }
        if (this->flamestate == 4) {
            this->y_bmp = 59;
        }
        if (this->flamestate == 5) {
            this->y_bmp = 30;
        }
    } else {
        this->y_bmp = 1;
    }
    this->x_bmp = 1 + ((this->shipframe) * (29));

    this->engine_on = 0;
    this->fx = 0;
    this->fy = 0;
}

void Player::Remove(ClientID id) {
    for (auto it : instances) {
        if (it.first == id) {
            // Delete player object
            delete (it.second);
            // Erase reference
            instances.erase(it.first);
            return;
        }
    }
    log::error("could not remove player with ID ", id,
               ", player does not exist");
}

// Write a playerstate message for every player instance to a session
void Player::EmitStates(ShipzSession *session) {
    for (auto it : Player::instances) {
        auto player = it.second;
        session->Write<SyncPlayerState>(
            player->player_id, player->status, player->typing,
            SignedFloatToNet(player->angle), UnsignedFloatToNet(player->x),
            UnsignedFloatToNet(player->y), SignedFloatToNet(player->vx),
            SignedFloatToNet(player->vy));
    }
}

void Player::Respawn() {
    this->shipframe = 0;
    this->flamestate = 0;
    this->x = 320;
    this->y = 290;
    this->angle = 0;
    this->vx = 0;
    this->vy = 0;
    this->fx = 0;
    this->fy = 0;
    this->crossx = 0;
    this->crossy = -CROSSHAIRDIST;
    this->weapon = WEAPON_BULLET;
}

// Update all player instances
void Player::UpdateAll(float delta) {
    for (auto it : instances) {
        auto player = it.second;
        // log::debug("Updating ", player->name);
        player->Update(delta);
    }
}

void TestColmaps() {
    SDL_Surface *test_map;

    char tmpfilename[100];

    memset(tmpfilename, '\0', sizeof(tmpfilename));
    snprintf(tmpfilename, 100, "%s./gfx/%s", SHAREPATH, "ship_collision.bmp");

    test_map = SDL_LoadBMP(tmpfilename);
    test_map = SDL_ConvertSurface(test_map, SDL_PIXELFORMAT_RGBA8888);
    SDL_SaveBMP(test_map, "testoutput.bmp");

    std::cout << "testing colmaps" << std::endl;
    for (int x = 0; x < test_map->w; x++) {
        for (int y = 0; y < test_map->h; y++) {
            // std::cout << a << b;
            if (GetPixel(test_map, x, y)) {
                std::cout << "1";
            } else
                std::cout << "0";
        }
        std::cout << std::endl;
    }
}

void GetCollisionMaps(bool **levelcolmap) {
    // loads the levelcollisionmaps and shipcollisionmap from their image files.

    SDL_Surface *ship_collision;
    SDL_Surface *level_collision;

    ship_collision = LoadBMP("ship_collision.bmp");
    level_collision = LoadBMP(lvl.m_colmap_filename.c_str());

    for (int a = 0; a < lvl.m_width; a++) {
        for (int b = 0; b < lvl.m_height; b++) {
            levelcolmap[a][b] = GetPixel(level_collision, a, b);
        }
    }

    for (int a = 0; a < 36; a++) {
        for (int b = 0; b < 28; b++) {
            for (int c = 0; c < 28; c++) {
                shipcolmap[a][b][c] =
                    GetPixel(ship_collision, ((a * 29) + 1 + b), c);
            }
        }
    }

    SDL_DestroySurface(level_collision);
    SDL_DestroySurface(ship_collision);
}

bool PlayerCollideWithLevel(Player *play, bool **levelcolmap) {
    // this function checks whether a players has crossed the level borders, or
    // crashed with the level. in both cases it returns 1.

    if (play->x > (lvl.m_width - 14)) {
        return true;
    }
    if (play->x < 14) {
        return true;
    }
    if (play->y > (lvl.m_height - 14)) {
        return true;
    }
    if (play->y < 14) {
        return true;
    }

    // fix this awful loop
    for (int a = 0; a < 28; a++) {
        for (int b = 0; b < 28; b++) {
            if (levelcolmap[int(play->x) + a - 14][int(play->y) + b - 14] &&
                shipcolmap[play->shipframe][a][b]) {
                // player collided with level
                return true;
            }
        }
    }
    // player didn't collide with level
    return false;
}

int PlayerCollideWithBullet(Player *play, int playernum, Player *players) {
    // TODO: Reimplement for Objects
    // // this function checks whether a player has collided with a bullet.
    // // if so, it returns the bullet number. if not it returns -1.

    // // NOTE: a better approach maybe would be to do the bullet tagging in
    // this function, so when 2 bullets hit a ship
    // // at the same time they both get erased. do this later (!)

    // int cb = 0;
    // for( cb = 0; cb < NUMBEROFBULLETS; cb++ )
    // {
    // 	if( players[ bullets[cb].owner - 1 ].team != players[ playernum - 1
    // ].team )
    // 	{
    // 		if( bullets[cb].type == WEAPON_ROCKET || bullets[cb].type ==
    // WEAPON_BULLET)
    // 		{
    // 			// first check if the bullet even is in the ships
    // clipping rectangle, this saves time. 			if(
    // bullets[cb].x > ( play->x - 1 )
    // && bullets[cb].x < ( play->x + 29 )
    // 			&& bullets[cb].y > ( play->y - 1 ) && bullets[cb].y < (
    // play->y + 29 ))
    // 			{
    // 				// the bullet seems to be in the ships clipping
    // rectangle,
    // 				// check for pixel-precise collisions..
    // 				// calculate the distance between bullet and
    // ship so we can see how far
    // 				// the bullet has entered the ships 'domain'
    // 				int dx = int( bullets[cb].x - play->x );
    // 				int dy = int( bullets[cb].y - play->y );
    // 				if( dx > 0 && dx < 28 && dy > 0 && dy < 28 )
    // 				{
    // 					if( shipcolmap[ dx ][ dy ] )
    // 					{
    // 						return cb;
    // 					}
    // 					if( dx < 27 )	// if we don't do this
    // might get a segsev
    // 					{
    // 						if( shipcolmap[play->shipframe][
    // dx + 1 ][ dy ] )
    // 						{
    // 							// return the bullet
    // number, so the main function can use
    // 							// it to tag the bullet
    // etc. same for the next 2 returns.
    // return cb;
    // 						}
    // 					}
    // 					if( dy < 27 )	// same here..
    // 					{
    // 						if( shipcolmap[play->shipframe][
    // dx ][ dy + 1 ] )
    // 						{
    // 							return cb;
    // 						}
    // 					}
    // 					if( dy < 27 && dx < 27 ) // and here..
    // 					{
    // 						if( shipcolmap[play->shipframe][
    // dx + 1 ][ dy + 1 ] )
    // 						{
    // 							return cb;
    // 						}
    // 					}
    // 				}
    // 			}
    // 		}
    // 	}
    // 	if( bullets[cb].type == WEAPON_MINE )
    // 	{
    // 		int dx = int( bullets[cb].x - play->x );
    // 		int dy = int( bullets[cb].y - play->y );
    // 		if( sqrt( dx*dx + dy*dy ) < MINEDETONATERADIUS &&
    // 				SDL_GetTicks() - bullets[cb].minelaidtime >
    // MINEACTIVATIONTIME )
    // 		{
    // 			return cb;
    // 		}
    // 	}
    // }
    // // if this point is reached, no bullet has collided with the players'
    // ship, so apparently he is safe..
    // // FOR NOW!!! NEXT TIME GADGET! NEXT TIME!!!!!!!!!!
    return -1;  // return no bullet collide signal.
}

int PlayerCollideWithBase(Player *play) {
    // // returns the number of the Base that is touching, if no Base is
    // touching, returns -1 int i; for( i = 0; i < lvl.m_num_bases; i++ )
    // {
    // 	if( int( play->y ) < bases[i].y-33 ||
    // 	    int( play->y ) > bases[i].y+17 ||
    // 	    int( play->x ) < bases[i].x-37 ||
    // 	    int( play->x ) > bases[i].x+37 )
    // 	{
    // 		// player isn't touching Base for sure
    // 	}
    // 	else
    // 	{
    // 		// there is a chance player is touching Base
    // 		for( int a =0 ; a < 28 ; a++ )
    // 		{
    // 			for( int b = 0; b < 28; b++)
    // 			{
    // 				if( int(play->x)+a-14 > bases[i].x - 21 &&
    // int(play->x)+a-14 < bases[i].x + 22 &&
    // int(play->y)+b-14 > bases[i].y - 18 && int(play->y)+b-14 < bases[i].y - 7
    // && shipcolmap[play->shipframe][a][b] )
    // 				{
    // 					//player collided with level
    // 					return i; // return the number of the
    // Base.
    // 				}
    // 			}
    // 		}
    // 	}

    // }
    // // for comment see previous function
    return -1;
}

void AdjustViewport(Player *play) {
    // this function focusses the viewport on a player. usually this is the
    // normal player, but it can also be used for spectator view or things like
    // that
    int tempx = int(play->x), tempy = int(play->y);
    viewportx = tempx - 320;
    viewporty = tempy - 240;
    if (viewportx < 0) {
        viewportx = 0;
    }
    if (viewporty < 0) {
        viewporty = 0;
    }
    if (viewportx > (lvl.m_width - XRES - 1)) {
        viewportx = lvl.m_width - XRES - 1;
    }
    if (viewporty > (lvl.m_height - YRES - 1)) {
        viewporty = lvl.m_height - YRES - 1;
    }
}

// rotates a player, scaled by the deltatime interval.
// clockwise flag determines direction of rotation
void Player::Rotate(bool clockwise, float delta) {
    if (clockwise) {
        this->angle += ROTATIONSPEED * delta;
        if (this->angle > 359) {
            this->angle -= 360;
        }
        this->crossy = look_sin[ConvertAngle(this->angle)] * -CROSSHAIRDIST;
        this->crossx = look_cos[ConvertAngle(this->angle)] * -CROSSHAIRDIST;
    } else {
        this->angle -= ROTATIONSPEED * delta;
        if (this->angle < 0) {
            this->angle += 360;
        }
        this->crossy = look_sin[ConvertAngle(this->angle)] * -CROSSHAIRDIST;
        this->crossx = look_cos[ConvertAngle(this->angle)] * -CROSSHAIRDIST;
    }
}

// Apply thrust for a player
void Player::Thrust() {
    this->fy = look_sin[ConvertAngle(this->angle)] * -THRUST;
    this->fx = look_cos[ConvertAngle(this->angle)] * -THRUST;
}

Player *GetNearestEnemyPlayer(int x, int y, int team) { return NULL; }

void Player::ResetForces() {
    this->fx = 0;
    this->fy = 0;
}

int GetNearestEnemyPlayer(Player *plyrs, int x, int y, int pteam) {
    // // returns a number to the enemy player nearest to x,y
    // int i; // loop thingy
    // int tdx, tdy; // temp dist
    // float dist = 1000000; // dist to nearest player, arbitrarily set very
    // high. float tdist; // temp value for calc of dist. int plr=-2; // the
    // nearest player so far

    // for( i = 0; i < MAXPLAYERS; i++ )
    // {
    // 	if( plyrs[i].playing && plyrs[i].status == PLAYER_STATUS::FLYING  )
    // 	{
    // 		tdx = int(x - plyrs[i].x);
    // 		tdy = int(y - plyrs[i].y);

    // 		tdist = sqrt(( tdx * tdx ) + ( tdy * tdy ));
    // 		if( tdist < dist && plyrs[i].team != pteam )
    // 		{
    // 			dist = tdist;
    // 			plr = i;
    // 		}

    // 	}
    // }
    // return plr+1;
    return 0;
}

void CheckBulletCollides(bool **colmap) {
    // checks if any active bullets have collided with the level, and, if they
    // have, tags them. int i; for( i = 0; i < NUMBEROFBULLETS; i++ )
    // {
    // 	if( bullets[i].active == true )
    // 	{
    // 		if( bullets[i].type == WEAPON_BULLET || bullets[i].type ==
    // WEAPON_ROCKET )
    // 		{
    // 			if( bullets[i].x > (lvl.m_width - 1) || bullets[i].x < 0
    // )
    // 			{
    // 				// std::cout << "bullet collided x" <<
    // std::endl; 				bullets[i].collide = true;
    // continue;
    // 			}
    // 			if( bullets[i].y > (lvl.m_height - 1) || bullets[i]. y <
    // 0 )
    // 			{
    // 				// std::cout << "bullet collided y" <<
    // std::endl; 				bullets[i].collide = true;
    // continue;
    // 			}
    // 			if( colmap[ int( bullets[i].x ) ][ int( bullets[i].y ) ]
    // == true )
    // 			{
    // 				// std::cout << "bullet " << i << " collided
    // colmap" << std::endl;
    // 				// std::cout << bullets[i].x << " " <<
    // bullets[i].y << std::endl;
    // 				// std::cout << int(bullets[i].x) << " " <<
    // int(bullets[i].y) << std::endl;
    // bullets[i].collide = true; 				continue;
    // 			}
    // 			if( colmap[ int( bullets[i].x ) + 1 ][ int( bullets[i].y
    // ) ] == true )
    // 			{
    // 				// std::cout << "bullet collided colmap 2" <<
    // std::endl; 				bullets[i].collide = true;
    // continue;
    // 			}
    // 			if( colmap[ int( bullets[i].x ) ][ int( bullets[i].y ) +
    // 1 ] == true )
    // 			{
    // 				// std::cout << "bullet collided colmap 3" <<
    // std::endl; 				bullets[i].collide = true;
    // continue;
    // 			}
    // 			if( colmap[ int( bullets[i].x ) + 1 ][ int( bullets[i].y
    // ) + 1 ] == true )
    // 			{
    // 				// std::cout << "bullet collided colmap 4" <<
    // std::endl; 				bullets[i].collide = true;
    // continue;
    // 			}
    // 		}
    // 		if( bullets[i].type == WEAPON_MINE )
    // 		{
    // 			if( SDL_GetTicks() - bullets[i].minelaidtime >
    // MINELIFETIME )
    // 			{
    // 				bullets[i].collide = true;
    // 			}
    // 		}
    // 	}
    // }
}

// Draw a player
void Player::Draw() {
    if (this->status == PLAYER_STATUS::DEAD) {
        return;
    }

    // Move to player
    switch (this->team) {
        case SHIPZ_TEAM::RED:
            DrawPlayer(shipred, this);
            return;
        case SHIPZ_TEAM::BLUE:
            DrawPlayer(shipblue, this);
            return;
    }
}

// Return how many players there are in a given team
uint16_t Player::GetTeamCount(TeamID team_id) {
    uint16_t count = 0;
    for (auto player : instances) {
        if (player.second->team == team_id) {
            count++;
        }
    }
    return count;
}