#ifndef SHIPZ_EVENT_H
#define SHIPZ_EVENT_H

#include "message_factory.h"

#define BASE_CLASS_NAME Event
#define BASE_CLASS_HEADER EVENT

//      Base class  Class name   Header
#define MESSAGE_CLASS_LIST(class_handler) \
    class_handler(PlayerJoins,   PLAYER_JOINS) \
    class_handler(PlayerLeaves,  PLAYER_LEAVES) \
    class_handler(TeamWins,      TEAM_WINS) \
    class_handler(LevelChanges,  LEVEL_CHANGE) \
    class_handler(ServerQuits,   SERVER_QUIT) \

#define FIELDS_PlayerJoins(field_handler) \
    field_handler(FIELD_UINT8, player_number)\
    field_handler(FIELD_STRING, player_name)

#define FIELDS_PlayerLeaves(field_handler) \
    field_handler(FIELD_UINT8, player_number)\
    field_handler(FIELD_STRING, leave_reason)

#define FIELDS_TeamWins(field_handler) \
    field_handler(FIELD_UINT8, team)\

#define FIELDS_LevelChanges(field_handler) \
    field_handler(FIELD_STRING, filename)\
    field_handler(FIELD_STRING, message)

#define FIELDS_ServerQuits(field_handler) \
    field_handler(FIELD_STRING, message)

MESSAGE_FACTORY_HEADER

#endif