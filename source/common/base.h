#ifndef SHIPZ_BASE_H
#define SHIPZ_BASE_H

#include <vector>
#include "common/types.h"
#include "common/object.h"
#include "messages/event.h"
#include "client/renderable.h"

#define STARTING_BASE_HEALTH 1000
#define BASE_SPAWN_Y_DELTA 26

class Base : public Object, public Renderable {
	public:
		uint16_t owner; // RED, BLUE or NEUTRAL
		uint16_t x;
		uint16_t y;
		uint16_t health;

		static std::vector<Base*> all_bases;

		static uint16_t GetTeamCount(TeamID team_id);

		Base(uint16_t owner, uint16_t x, uint16_t y);
		Base(EventObjectSpawn * sync);

		std::shared_ptr<EventObjectSpawn> EmitSpawnMessage();

		// Get number of bases owned by a team
		static int BasesOwned(int owning_team);

		// Get random base
		static ObjectID GetRandomRespawnBase(int owning_team);

		void Sync(SyncObjectUpdate *sync);
		void Draw();
};

#endif