#ifndef SHIPZ_BASE_H
#define SHIPZ_BASE_H

#include "types.h"
#include "object.h"

struct Base : public Object {
	int owner; // RED, BLUE or NEUTRAL
	int x;
	int y;
	int health;
};

extern Base bases[MAXBASES];

#endif