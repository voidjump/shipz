#include "base.h"
#include "team.h"
#include "gfx.h"
#include "assets.h"

// Draw a base
void Base::Draw() {
    auto index = 0;
    switch(this->owner) {
        case SHIPZ_TEAM::RED:
            index = 0;
            break;
        case SHIPZ_TEAM::BLUE:
            index = 19;
            break;
        case SHIPZ_TEAM::NEUTRAL:
            index = 38;
            break;
        default:
            return;
    }
    DrawIMG( basesimg, this->x - 20 - viewportx,
            this->y - 17 - viewporty, 41, 18, 0, index );
}

// Construct a rocket from a spawn message
Base::Base(uint16_t owner, uint16_t x, uint32_t y) : Object(sync->id, OBJECT_TYPE::ROCKET) {
	this->owner = owner;
	this->x = x;
	this->y = y;
	this->health = STARTING_BASE_HEALTH;

    this->destroy_callback = nullptr;
    this->update_callback = nullptr;
    this->sync_callback = std::bind(&Rocket::Sync, this, std::placeholders::_1);
}


// Synchronize base status with server
void Base::Sync(SyncObjectUpdate *sync) {
	x = NetToUnsignedFloat(pop_uint16(sync->data));
	y = NetToUnsignedFloat(pop_uint16(sync->data));
	owner = NetToUnsignedFloat(pop_uint16(sync->data));
	health = NetToUnsignedFloat(pop_uint16(sync->data));
}

void Rocket::Draw() {
	int ta = int( this->angle ) / 10;
	int x = (ta * 14) + ta +1;
	DrawIMG(rocketpixmap, int(this->x - 7 - viewportx),
		int(this->y -7 - viewporty), 14, 14, x, 1);
}
