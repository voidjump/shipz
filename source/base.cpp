#include "base.h"
#include "team.h"
#include "gfx.h"
#include "assets.h"

// all instances
std::vector<Base*> Base::all_bases;

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

// Construct a base locally (use for server)
Base::Base(uint16_t owner, uint16_t x, uint32_t y) : Object(OBJECT_TYPE::BASE) {
	this->owner = owner;
	this->x = x;
	this->y = y;
	this->health = STARTING_BASE_HEALTH;

    this->destroy_callback = nullptr;
    this->update_callback = nullptr;
    this->sync_callback = nullptr;

    all_bases.push_back(this);
}

// Construct a base from a spawn message
Base::Base(SyncObjectSpawn * sync) : Object(sync->id, OBJECT_TYPE::BASE) {
	owner = pop_uint16(sync->data);
	x = pop_uint16(sync->data);
	y = pop_uint16(sync->data);
	health = pop_uint16(sync->data);

    this->destroy_callback = nullptr;
    this->update_callback = nullptr;
    this->sync_callback = std::bind(&Base::Sync, this, std::placeholders::_1);

    all_bases.push_back(this);
}

// Synchronize base status with server
void Base::Sync(SyncObjectUpdate *sync) {
    owner = pop_uint16(sync->data);
	x = pop_uint16(sync->data);
	y = pop_uint16(sync->data);
	health = pop_uint16(sync->data);
}

