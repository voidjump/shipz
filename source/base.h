#ifndef SHIPZ_BASE_H
#define SHIPZ_BASE_H

#include "types.h"

struct Base
{
	bool used; // is this Base used or not used?
	int owner; // RED, BLUE or NEUTRAL
	int x;
	int y;
	int health;
};

extern Base bases[MAXBASES];

#endif