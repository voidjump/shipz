#ifndef SHIPZPLAYER_H
#define SHIPZPLAYER_H

#include "types.h"
void TestColmaps();
const char * GetStatusString(int status);
void EmptyPlayer( Player * play );
inline int ConvertAngle( float angle );
void InitPlayer( Player * play );
void UpdatePlayer( Player * play );
void ResetPlayer( Player * play );
void GetCollisionMaps( bool ** levelcolmap );
bool PlayerCollideWithLevel( Player * play, bool ** levelcolmap );
int PlayerCollideWithBullet( Player * play, int playernum, Player * players );
int PlayerCollideWithBase( Player * play );
void AdjustViewport( Player * play );
int GetNearestEnemyPlayer( Player * plyrs, int x, int y, int pteam );
void PlayerRot( Player * play, bool clockwise );
void PlayerThrust( Player * play );
Uint16 ShootBullet( Player * play, int owner );
int FindRespawnBase( int rspwnteam );
void UpdateBullets( Player * plyrs );
void CheckBulletCollides( bool ** colmap );
void CleanBullet( int num );
#endif
