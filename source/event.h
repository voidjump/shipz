#ifndef SHIPZ_EVENT_H
#define SHIPZ_EVENT_H

#include "message_factory.h"

#define BASE_CLASS_NAME Event
#define BASE_CLASS_HEADER EVENT

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
    class_handler(Chat,       CHAT_ALL) \
    class_handler(TeamWins,      TEAM_WINS) \
    class_handler(LevelChanges,  LEVEL_CHANGE) \
    class_handler(ServerQuits,   SERVER_QUIT) \

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


MESSAGE_FACTORY_HEADER

#endif