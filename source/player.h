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

struct Player
{
	bool playing; 
	int kills, deaths, Team;
	char name[13];
	int flamestate, shipframe;
	int weapon; // weapon player is 'carrying'
	int status;
	float x, y, vx, vy, fx, fy, angle;
	bool engine_on;
	int y_bmp, x_bmp;
	float crossx, crossy; // x and y of the crosshair
	bool self_sustaining; // is the player local or remote?
	SDLNet_Address * playaddr;
	float lastsendtime;

	bool typing; // is the player typing a message?

	float lastliftofftime;
	bool bullet_shot;
	Uint16 bulletshotnr;
	float lastshottime;
};


void TestColmaps();
const char * GetStatusString(int status);
void EmptyPlayer( Player * play );
inline int ConvertAngle( float angle );
void InitPlayer( Player * play );
void UpdatePlayer( Player * play );
void ResetPlayer( Player * play );
void GetCollisionMaps( bool ** levelcolmap );
bool PlayerCollideWithLevel( Player * play, bool ** levelcolmap );
int PlayerCollideWithBullet( Player * play, int playernum, Player * players );
int PlayerCollideWithBase( Player * play );
void AdjustViewport( Player * play );
int GetNearestEnemyPlayer( Player * plyrs, int x, int y, int pteam );
void PlayerRot( Player * play, bool clockwise );
void PlayerThrust( Player * play );
Uint16 ShootBullet( Player * play, int owner );
int FindRespawnBase( int rspwnteam );
void UpdateBullets( Player * plyrs );
void CheckBulletCollides( bool ** colmap );
void CleanBullet( int num );
#endif
