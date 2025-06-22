#ifndef SHIPZ_EVENT_H
#define SHIPZ_EVENT_H

#include "net/message_factory.h"
 
#undef BASE_CLASS_NAME
#undef BASE_CLASS_HEADER
#undef BASE_DEFAULT_RELIABLE
#undef MESSAGE_CLASS_LIST
#define BASE_CLASS_NAME Event
#define BASE_CLASS_HEADER EVENT
#define BASE_DEFAULT_RELIABLE true

/* Event messages
 * 
 * General state changes that need to be reliablably synchronized.
 * 
 * Generally involves all connected clients;
 **/

#define MESSAGE_CLASS_LIST(class_handler) \
    class_handler(PlayerJoins,   PLAYER_JOINS) \
    class_handler(PlayerLeaves,  PLAYER_LEAVES) \
    class_handler(PlayerKicked,  PLAYER_KICKED) \
    class_handler(Chat,          CHAT_ALL) \
    class_handler(TeamWins,      TEAM_WINS) \
    class_handler(LevelChanges,  LEVEL_CHANGE) \
    class_handler(ServerQuits,   SERVER_QUIT) \
    class_handler(ObjectSpawn,           OBJECT_SPAWN) \
    class_handler(ObjectDestroy,         OBJECT_DESTROY) \
    class_handler(PlayerLiftOff,         PLAYER_LIFTOFF) \
    class_handler(PlayerSpawn,         PLAYER_SPAWN)

// Another player has joined
#define FIELDS_PlayerJoins(field_handler) \
    field_handler(FIELD_UINT8, client_id) \
    field_handler(FIELD_UINT8, team) \
    field_handler(FIELD_STRING, player_name) 

// Another player has left the game
#define FIELDS_PlayerLeaves(field_handler) \
    field_handler(FIELD_UINT8, client_id)\
    field_handler(FIELD_STRING, leave_reason)

// A player is kicked 
#define FIELDS_PlayerKicked(field_handler) \
    field_handler(FIELD_UINT8, client_id)

// Someone sent a chat message
#define FIELDS_Chat(field_handler) \
    field_handler(FIELD_STRING, message) \
    field_handler(FIELD_UINT8, client_id)\
    field_handler(FIELD_UINT8, team) 

// One of the Teams has won the game
#define FIELDS_TeamWins(field_handler) \
    field_handler(FIELD_UINT8, team)\

// Client should load a new level (Should this be a CMD type?) 
#define FIELDS_LevelChanges(field_handler) \
    field_handler(FIELD_STRING, filename)\
    field_handler(FIELD_STRING, message)

// The Server is quitting the game
#define FIELDS_ServerQuits(field_handler) \
    field_handler(FIELD_STRING, message)

// An object is being created
#define FIELDS_ObjectSpawn(field_handler) \
    field_handler(FIELD_UINT16,         id) \
    field_handler(FIELD_UINT8,        type) \
    field_handler(FIELD_UINT8,        size) \
    field_handler(FIELD_OCTETS,       data)

// An object is being destroyed
#define FIELDS_ObjectDestroy(field_handler) \
    field_handler(FIELD_UINT16,         id)

// A player respawns at a base
#define FIELDS_PlayerLiftOff(field_handler) \
    field_handler(FIELD_UINT16,         client_id) 

// A player respawns at a base
#define FIELDS_PlayerSpawn(field_handler) \
    field_handler(FIELD_UINT16,         client_id) \
    field_handler(FIELD_UINT16,         base_id)

MESSAGE_FACTORY_HEADER

#endif