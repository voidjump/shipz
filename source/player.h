#ifndef SHIPZPLAYER_H
#define SHIPZPLAYER_H

#include "types.h"

void EmptyPlayer( player * play );
inline int ConvertAngle( float angle );
void InitPlayer( player * play );
void UpdatePlayer( player * play );
void ResetPlayer( player * play );
void GetCollisionMaps( bool ** levelcolmap );
bool PlayerCollideWithLevel( player * play, bool ** levelcolmap );
int PlayerCollideWithBullet( player * play, int playernum, player * players );
int PlayerCollideWithBase( player * play );
void AdjustViewport( player * play );
int GetNearestEnemyPlayer( player * plyrs, int x, int y, int pteam );
void PlayerRot( player * play, bool clockwise );
void PlayerThrust( player * play );
Uint16 ShootBullet( player * play, int owner );
int FindRespawnBase( int rspwnteam );
void UpdateBullets( player * plyrs );
void CheckBulletCollides( bool ** colmap );
void CleanBullet( int num );
#endif
