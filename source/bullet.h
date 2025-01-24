#ifndef SHIPZ_BULLET_H
#define SHIPZ_BULLET_H

#include <SDL3/SDL.h>
#include "object.h"
#include "player.h"
#include "renderable.h"

#define ROCKETRADARRADIUS 300
#define MAXROCKETTURN 50
#define ROCKETSPEED 95

class Bullet : public Object, public Renderable {
    public:
	float x, y, angle; // coordinates and angle (bullets always have same speed)
	Uint16 owner; // number of player who shot the bullet

    Bullet(SyncObjectSpawn *sync);
    static SyncObjectSpawn * Shoot(Player *self);
    void Update(float delta);
    void Draw() override;
};

class Rocket : public Object, public Renderable {
    public:
	float x, y, angle; // coordinates & speed
	Uint16 owner; // number of player who shot the bullet

    static Player * players;
    

    void TurnToNearest(Player *nearest, float delta);
    Rocket(SyncObjectSpawn *sync);
    static SyncObjectSpawn * Shoot(Player *self);
    void Update(float delta);
    void Sync(SyncObjectUpdate *sync);
    void Draw() override;
};

class Mine : public Object, public Renderable {
    public:
	float x, y; // coordinates
	float minelaidtime;

    Mine(SyncObjectSpawn *sync);
    static SyncObjectSpawn * Shoot(Player *self);
    void Draw() override;
};

#endif