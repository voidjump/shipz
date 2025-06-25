#include "common/base.h"

#include <random>

#include "common/assets.h"
#include "client/gfx.h"
#include "common/team.h"

// all instances
std::vector<Base *> Base::all_bases;

// Draw a base
void Base::Draw() {
#ifdef CLIENT
    auto index = 0;
    switch (this->owner) {
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
    DrawIMG(basesimg, this->x - 20 - viewportx, this->y - 17 - viewporty, 41,
            18, 0, index);
#endif
}

// Construct a base locally (use for server)
Base::Base(uint16_t owner, uint16_t x, uint16_t y) : Object(OBJECT_TYPE::BASE) {
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
Base::Base(EventObjectSpawn *sync) : Object(sync->id, OBJECT_TYPE::BASE) {
    owner = pop_uint16(sync->data);
    x = pop_uint16(sync->data);
    y = pop_uint16(sync->data);
    health = pop_uint16(sync->data);

    this->destroy_callback = nullptr;
    this->update_callback = nullptr;
    this->sync_callback = std::bind(&Base::Sync, this, std::placeholders::_1);

    logger::debug("Spawned base at ", x, ",", y);
    all_bases.push_back(this);
}

// Synchronize base status with server
void Base::Sync(SyncObjectUpdate *sync) {
    owner = pop_uint16(sync->data);
    x = pop_uint16(sync->data);
    y = pop_uint16(sync->data);
    health = pop_uint16(sync->data);
}

// Return how many bases this team owns
int Base::BasesOwned(int owning_team) {
    int count = 0;
    for (auto base : all_bases) {
        if (base->owner == owning_team) {
            count++;
        }
    }
    return count;
}

// Get random base
ObjectID Base::GetRandomRespawnBase(int owning_team) {
    std::random_device rand_dev;

    auto count = Base::BasesOwned(owning_team);

    // select a random base (0-indexed)
    auto selected_base = rand_dev() % count;
    unsigned int idx = 0;

    for (auto base : all_bases) {
        if (base->owner == owning_team) {
            if (selected_base == idx) {
                return dynamic_cast<Object *>(base)->id;
            }
            idx ++;
        }
    }
    // This should not happen
    return 0;
}


std::shared_ptr<EventObjectSpawn> Base::EmitSpawnMessage() {
	std::vector<Uint8> data;
	append_to_object(data, this->owner);
	append_to_object(data, this->x);
	append_to_object(data, this->y);
	append_to_object(data, this->health);

    logger::debug("BASE WITH ID", this->id);
    logger::debug("x", this->x);
    logger::debug("y", this->y);
	auto sync = std::make_shared<EventObjectSpawn>(this->id,
												OBJECT_TYPE::BASE,
												8,
												data);
	return sync;
}

uint16_t Base::GetTeamCount(TeamID team_id) {
    uint16_t count = 0;
    for(auto base : all_bases) {
        if(base->owner == team_id) {
            count++;
        }
    }
    return count;
}