#ifndef SHIPZPLAYER_H
#define SHIPZPLAYER_H

#include <map>
#include "base.h"
#include "session.h"
#include "sync.h"
#include "types.h"

using PlayerID = uint16_t;

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
		static std::map<PlayerID, Player*> instances;

		PlayerID player_id;
		// The session associated with this player
		ShipzSession * session;

		// the player name
		std::string name;

		int kills, deaths, team;

		int flamestate, shipframe;
		// weapon player is currently using
		int weapon; 
		// status
		uint16_t status;
		// position, speed, forces, angle
		float x, y, vx, vy, fx, fy, angle;
		// whether the player is currently thrusting
		bool engine_on;

		int y_bmp, x_bmp;
		// x and y of the crosshair
		float crossx, crossy; 
		// is the player local or remote?
		bool self_sustaining; 

		// is the player typing a message?
		// TODO: this could be a bitflag in the player state
		bool typing; 

		// Last time player sent an update
		float lastsendtime;
		// last time player lifted off
		float lastliftofftime;
		// last time player shot a bullet
		float lastshottime;

	// Client side
	Player(uint16_t id);
	// Server side
	Player(ShipzSession * session);
	~Player();

	void HandleUpdate(SyncPlayerState *sync);
	PlayerID GeneratePlayerID();

	// Retrieve a player instance by their ID
	static Player * GetByID(uint16_t search_id);
	void Init();
	void Empty();
	void Update();
	void Respawn();
	void Rotate( bool clockwise );
	void Thrust();
	void Draw();
};


inline int ConvertAngle( float angle )
{
	//converts an angle from float/game position ( 0 up ) to integer standard position ( 0 right )
	int temp_angle;
	temp_angle = int( angle ) + 90; // 0 degrees is right normall, we'll make it up.
	if( temp_angle > 359 )
		temp_angle -= 360;
	return temp_angle;
}
void TestColmaps();
const char * GetStatusString(int status);
void GetCollisionMaps( bool ** levelcolmap );
bool PlayerCollideWithLevel( Player * play, bool ** levelcolmap );
int PlayerCollideWithBullet( Player * play, int playernum, Player * players );
int PlayerCollideWithBase( Player * play );
void AdjustViewport( Player * play );
Player* GetNearestEnemyPlayer( int x, int y ,int team );
int GetNearestEnemyPlayer( Player * plyrs, int x, int y, int pteam );
Uint16 ShootBullet( Player * play, int owner );
Base * FindRespawnBase( int rspwnteam );
void UpdateBullets( Player * plyrs );
void CheckBulletCollides( bool ** colmap );
#endif
