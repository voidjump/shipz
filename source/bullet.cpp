#include <functional>
#include "sync.h"
#include "bullet.h"
#include "object.h"
#include "types.h"
#include "player.h"
#include "log.h"

// TODO Add constructor for local creation
// Construct a bullet from a spawn message
Bullet::Bullet(SyncObjectSpawn *sync) : Object(sync->id, OBJECT_TYPE::BULLET) {
	Uint16 *values = (Uint16*)sync->data;
	x = NetToUnsignedFloat(values[0]);
	y = NetToUnsignedFloat(values[1]);
	angle = NetToUnsignedFloat(values[2]);

    this->sync_callback = nullptr;
    this->destroy_callback = nullptr;
    this->update_callback = std::bind(&Bullet::Update, this, std::placeholders::_1);
}

// Update bullet path in straight line
void Bullet::Update(float delta) {
	x -= look_cos[ConvertAngle( angle )] * BULLETSPEED * (delta/1000);
	y -= look_sin[ConvertAngle( angle )] * BULLETSPEED * (delta/1000);
}

// Construct a rocket from a spawn message
Rocket::Rocket(SyncObjectSpawn *sync) : Object(sync->id, OBJECT_TYPE::ROCKET) {
	Uint16 *values = (Uint16*)sync->data;
	x = NetToUnsignedFloat(values[0]);
	y = NetToUnsignedFloat(values[1]);
	angle = NetToUnsignedFloat(values[2]);

    this->destroy_callback = nullptr;
    this->update_callback = std::bind(&Rocket::Update, this, std::placeholders::_1);
    this->sync_callback = std::bind(&Rocket::Sync, this, std::placeholders::_1);
}

// Synchronize rocket status with server
void Rocket::Sync(SyncObjectUpdate *sync) {
	Uint16 *values = (Uint16*)sync->data;
	x = NetToUnsignedFloat(values[0]);
	y = NetToUnsignedFloat(values[1]);
	angle = NetToUnsignedFloat(values[2]);
}

void Rocket::TurnToNearest(Player *nearest, float delta) {
	int xd = int(x - nearest->x);
	int yd = int(y - nearest->y);

	float distance, dirz, Tx, Ty, Rx, Ry, CrossProd;
		
	distance = (float) sqrt(xd * xd + yd * yd);
	if( ( distance > ROCKETRADARRADIUS )) // can the rocket see the player??
	{
		return;
	}
	// target vector
	Tx = xd/distance;
	Ty = yd/distance;
	// rocket direction vector
	Rx = look_cos[ConvertAngle( angle )];
	Ry = look_sin[ConvertAngle( angle )];
	// calculate the cross product
	CrossProd = ( Tx * Ry - Rx * Ty );
	if( CrossProd > 0 )
	{
		// rotate counterclockwise
		angle -= MAXROCKETTURN * (delta/1000);
		if( angle < 0 )
		{
			angle += 359;
		}
	} else {
		// rotate clockwise
		angle += MAXROCKETTURN * (delta/1000);
		if( angle > 360 )
		{
			angle -= 359;
		}
	}
}

// Rockets always move to the nearest enemy player
void Rocket::Update(float delta) {
	Player * nearest = GetNearestEnemyPlayer( int(x),
			int(y), owner );
	if ( nearest != NULL )	// if no enemyplayer is found don't steer
	{
		TurnToNearest(nearest, delta);
	}
	// update rocket coordinates
	x -= look_cos[ConvertAngle( angle )] * ROCKETSPEED * (delta/1000);
	y -= look_sin[ConvertAngle( angle )] * ROCKETSPEED * (delta/1000);
}

// Spawn a mine from a sync message
Mine::Mine(SyncObjectSpawn *sync) : Object(sync->id, OBJECT_TYPE::MINE) {
	Uint16 *values = (Uint16*)sync->data;
	x = NetToUnsignedFloat(values[0]);
	y = NetToUnsignedFloat(values[1]);
    this->sync_callback = nullptr;
    this->destroy_callback = nullptr;
    this->update_callback = nullptr;
}