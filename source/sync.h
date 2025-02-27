#ifndef SHIPZ_SYNC_H
#define SHIPZ_SYNC_H

#include "message_factory.h"

/* 
 * Sync messages: Updates on game state
 *
 * Sync messages are generally unreliable
 * Mostly server -> clients
 */

#undef BASE_CLASS_NAME
#undef BASE_CLASS_HEADER
#undef BASE_DEFAULT_RELIABLE
#undef MESSAGE_CLASS_LIST
#define BASE_CLASS_NAME Sync 
#define BASE_CLASS_HEADER SYNC
#define BASE_DEFAULT_RELIABLE false

#define MESSAGE_CLASS_LIST(class_handler) \
    class_handler(PlayerState,           PLAYER_STATE) \
    class_handler(ObjectUpdate,          OBJECT_UPDATE) \
    class_handler(TeamStates,            TEAM_STATES) 

// This is player state shared from server to player
#define FIELDS_PlayerState(field_handler) \
    field_handler(FIELD_UINT16,   client_id) \
    field_handler(FIELD_UINT16,   status_bits) \
    field_handler(FIELD_UINT8,    typing) \
    field_handler(FIELD_UINT16,   angle) \
    field_handler(FIELD_UINT16,   x) \
    field_handler(FIELD_UINT16,   y) \
    field_handler(FIELD_UINT16,   vx) \
    field_handler(FIELD_UINT16,   vy)


#define FIELDS_ObjectUpdate(field_handler) \
    field_handler(FIELD_UINT16,         id) \
    field_handler(FIELD_UINT8,        size) \
    field_handler(FIELD_OCTETS,       data)


#define FIELDS_TeamStates(field_handler) \
    field_handler(FIELD_UINT32,   base_states) \
    field_handler(FIELD_UINT8,   red_kills) \
    field_handler(FIELD_UINT8,   blue_kills) 

MESSAGE_FACTORY_HEADER

#endif