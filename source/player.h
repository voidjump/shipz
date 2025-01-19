#ifndef SHIPZPLAYER_H
#define SHIPZPLAYER_H

#include "types.h"

enum PLAYER_STATUS {
 DEAD,
 FLYING, 
 LANDED,
 JUSTCOLLIDEDROCK,
 JUSTSHOT,
 RESPAWN,
 LIFTOFF,
 SUICIDE,
 JUSTCOLLIDEDBASE, 
 LANDEDBASE, 
 LANDEDRESPAWN,
};

enum PLAYER_WEAPON {
	WEAPON_BULLET = 1,
	WEAPON_ROCKET = 2,
 	WEAPON_MINE = 3,
};

class Player
{
	public:
		Uint16 client_id;

		// the player name
		std::string name;

		int kills, deaths, team;

		int flamestate, shipframe;
		// weapon player is currently using
		int weapon; 
		// status
		int status;
		// position, speed, forces, angle
		float x, y, vx, vy, fx, fy, angle;
		// whether the player is currently thrusting
		bool engine_on;

		int y_bmp, x_bmp;
		// x and y of the crosshair
		float crossx, crossy; 
		// is the player local or remote?
		bool self_sustaining; 
		SDLNet_Address * playaddr;

		// is the player typing a message?
		// TODO: this could be a bitflag in the player state
		bool typing; 

		bool bullet_shot;
		Uint16 bulletshotnr;

		// Last time player sent an update
		float lastsendtime;
		// last time player lifted off
		float lastliftofftime;
		// last time player shot a bullet
		float lastshottime;

	void Init();
	void Empty();
	void Update();
	void Respawn();
	void Rotate( bool clockwise );
	void Thrust();
};

void TestColmaps();
const char * GetStatusString(int status);
inline int ConvertAngle( float angle );
void GetCollisionMaps( bool ** levelcolmap );
bool PlayerCollideWithLevel( Player * play, bool ** levelcolmap );
int PlayerCollideWithBullet( Player * play, int playernum, Player * players );
int PlayerCollideWithBase( Player * play );
void AdjustViewport( Player * play );
Player* GetNearestEnemyPlayer( int x, int y ,int team );
int GetNearestEnemyPlayer( Player * plyrs, int x, int y, int pteam );
Uint16 ShootBullet( Player * play, int owner );
int FindRespawnBase( int rspwnteam );
void UpdateBullets( Player * plyrs );
void CheckBulletCollides( bool ** colmap );
void CleanBullet( int num );
#endif
