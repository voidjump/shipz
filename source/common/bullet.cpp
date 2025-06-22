#include <functional>
#include "messages/sync.h"
#include "common/bullet.h"
#include "common/object.h"
#include "common/types.h"
#include "common/player.h"
#include "utils/log.h"
#include "common/assets.h"
#include "client/gfx.h"

///////////////////////////////////////////////////////////////////////////////
// BULLET
///////////////////////////////////////////////////////////////////////////////

// Construct a bullet from a spawn message
Bullet::Bullet(EventObjectSpawn *sync) : Object(sync->id, OBJECT_TYPE::BULLET) {
	x = NetToUnsignedFloat(pop_uint16(sync->data));
	y = NetToUnsignedFloat(pop_uint16(sync->data));
	angle = NetToUnsignedFloat(pop_uint16(sync->data));

    this->sync_callback = nullptr;
    this->destroy_callback = nullptr;
    this->update_callback = std::bind(&Bullet::Update, this, std::placeholders::_1);
}

// Update bullet path in straight line
void Bullet::Update(float delta) {
	x -= look_cos[ConvertAngle( angle )] * BULLETSPEED * (delta/1000);
	y -= look_sin[ConvertAngle( angle )] * BULLETSPEED * (delta/1000);
}


// Shoot a bullet
EventObjectSpawn * Bullet::Shoot(Player *self) {
	std::vector<Uint8> data;
	append_to_object(data, self->angle);
	append_to_object(data, self->x);
	append_to_object(data, self->y);

	EventObjectSpawn *sync = new EventObjectSpawn(SERVER_SHOULD_DEFINE_ID,
												OBJECT_TYPE::BULLET,
												6,
												data);
	return sync;
}

void Bullet::Draw() {
	DrawIMG(bulletpixmap, int(this->x - viewportx), int(this->y - viewporty));
}

///////////////////////////////////////////////////////////////////////////////
// Rocket
///////////////////////////////////////////////////////////////////////////////

// Shoot a rocket
EventObjectSpawn * Rocket::Shoot(Player *self) {
	std::vector<Uint8> data;
	append_to_object(data, self->angle);
	append_to_object(data, self->x);
	append_to_object(data, self->y);

	EventObjectSpawn *sync = new EventObjectSpawn(SERVER_SHOULD_DEFINE_ID,
												OBJECT_TYPE::ROCKET,
												6,
												data);
	return sync;
}

// Construct a rocket from a spawn message
Rocket::Rocket(EventObjectSpawn *sync) : Object(sync->id, OBJECT_TYPE::ROCKET) {
	x = NetToUnsignedFloat(pop_uint16(sync->data));
	y = NetToUnsignedFloat(pop_uint16(sync->data));
	angle = NetToUnsignedFloat(pop_uint16(sync->data));

    this->destroy_callback = nullptr;
    this->update_callback = std::bind(&Rocket::Update, this, std::placeholders::_1);
    this->sync_callback = std::bind(&Rocket::Sync, this, std::placeholders::_1);
}

// Synchronize rocket status with server
void Rocket::Sync(SyncObjectUpdate *sync) {
	x = NetToUnsignedFloat(pop_uint16(sync->data));
	y = NetToUnsignedFloat(pop_uint16(sync->data));
	angle = NetToUnsignedFloat(pop_uint16(sync->data));
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

void Rocket::Draw() {
	int ta = int( this->angle ) / 10;
	int x = (ta * 14) + ta +1;
	DrawIMG(rocketpixmap, int(this->x - 7 - viewportx),
		int(this->y -7 - viewporty), 14, 14, x, 1);
}

///////////////////////////////////////////////////////////////////////////////
// Mine
///////////////////////////////////////////////////////////////////////////////

// Shoot a mine
EventObjectSpawn * Mine::Shoot(Player *self) {
	std::vector<Uint8> data;
	append_to_object(data, self->x);
	append_to_object(data, self->y);

	EventObjectSpawn *sync = new EventObjectSpawn(SERVER_SHOULD_DEFINE_ID,
												OBJECT_TYPE::MINE,
												4,
												data);
	return sync;
}

// Spawn a mine from a sync message
Mine::Mine(EventObjectSpawn *sync) : Object(sync->id, OBJECT_TYPE::MINE) {
	x = NetToUnsignedFloat(pop_uint16(sync->data));
	y = NetToUnsignedFloat(pop_uint16(sync->data));
    this->sync_callback = nullptr;
    this->destroy_callback = nullptr;
    this->update_callback = nullptr;
}

void Mine::Draw() {
	int flick2 = int(SDL_GetTicks() - this->minelaidtime);
	int flick = flick2;
	
	flick = flick%1000;
	flick2 = flick2%360;
	int mineyoffset = int( 3 * look_sin[flick2] );
	if( SDL_GetTicks() - this->minelaidtime > MINEACTIVATIONTIME )
	{
		if( flick > 500 )
		{
			DrawIMG(minepixmap, int(this->x - 6 - viewportx),
				int(this->y -6 - viewporty + mineyoffset), 13, 13, 0, 0);
		}
		else
		{
			DrawIMG(minepixmap, int(this->x - 6 - viewportx),
				int(this->y -6 - viewporty + mineyoffset), 13, 13, 13, 0);
		}
	}
	else
	{
		DrawIMG(minepixmap, int(this->x - 6 - viewportx),
			int(this->y -6 - viewporty ), 13, 13, 13, 0);

	}
}