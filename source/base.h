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

		Base(SyncObjectSpawn * sync) : Object(sync->id, OBJECT_TYPE::ROCKET);

		void Sync(SyncObjectUpdate *sync);
		void Draw();
};

#endif