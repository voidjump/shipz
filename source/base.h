#ifndef SHIPZ_BASE_H
#define SHIPZ_BASE_H

#include <vector>
#include "types.h"
#include "object.h"
#include "renderable.h"

#define STARTING_BASE_HEALTH 1000

class Base : public Object, public Renderable {
	public:
		int owner; // RED, BLUE or NEUTRAL
		int x;
		int y;
		int health;

		static std::vector<Base*> all_bases;

		Base(uint16_t owner, uint16_t x, uint32_t y);
		Base(SyncObjectSpawn * sync);

		void Sync(SyncObjectUpdate *sync);
		void Draw();
};

#endif