#ifndef SHIPZ_SYNC_H
#define SHIPZ_SYNC_H

#include "message_factory.h"

#define BASE_CLASS_NAME Sync 
#define BASE_CLASS_HEADER SYNC

#define MESSAGE_CLASS_LIST(class_handler) \
    class_handler(PlayerState,           PLAYER_STATE) \
    class_handler(ObjectSpawn,           OBJECT_SPAWN) \
    class_handler(ObjectUpdate,          OBJECT_UPDATE) \
    class_handler(ObjectDestroy,         OBJECT_DESTROY) \
    class_handler(TeamStates,            TEAM_STATES) \
    class_handler(BaseStates,            BASE_STATES)

// This is player state shared from server to player
#define FIELDS_PlayerState(field_handler) \
    field_handler(FIELD_UINT16,   status_bits) \
    field_handler(FIELD_UINT16,   angle) \
    field_handler(FIELD_UINT16,   x) \
    field_handler(FIELD_UINT16,   y) \
    field_handler(FIELD_UINT16,   vx) \
    field_handler(FIELD_UINT16,   vy)

#define FIELDS_ObjectSpawn(field_handler) \
    field_handler(FIELD_UINT16,         id) \
    field_handler(FIELD_SERIALIZABLE,   data)

#define FIELDS_ObjectUpdate(field_handler) \
    field_handler(FIELD_UINT16,         id) \
    field_handler(FIELD_SERIALIZABLE,   data)

#define FIELDS_ObjectDestroy(field_handler) \
    field_handler(FIELD_UINT16,         id)


// This is player state shared from server to player
#define FIELDS_TeamStates(field_handler) \
    field_handler(FIELD_UINT8,   red_kills) \
    field_handler(FIELD_UINT8,   blue_kills)

#define FIELDS_BaseStates(field_handler) \
    field_handler(FIELD_UINT32,   client_id)

MESSAGE_FACTORY_HEADER

#endif