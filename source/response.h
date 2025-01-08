#ifndef SHIPZ_RESPONSE_H
#define SHIPZ_RESPONSE_H

#include "message_factory.h"

#define BASE_CLASS_NAME Response 
#define BASE_CLASS_HEADER RESPONSE

#define MESSAGE_CLASS_LIST(class_handler) \
    class_handler(ServerInformation,    SERVER_INFO) \
    class_handler(PlayerInformation,    PLAYER_INFO) \
    class_handler(AcceptJoin,           ACCEPT_JOIN) \
    class_handler(AcknowledgeLeave,     ACK_LEAVE) \

// Server responds to a client's 'GET_SERVER_INFO' request
#define FIELDS_ServerInformation(field_handler) \
    field_handler(FIELD_UINT8, shipz_version) \
    field_handler(FIELD_UINT8, number_of_players) \
    field_handler(FIELD_UINT8, max_players) \
    field_handler(FIELD_UINT8, level_version) \
    field_handler(FIELD_STRING, level_filename)

// Server responds to a client's 'JOIN_GAME' request
#define FIELDS_AcceptJoin(field_handler) \
    field_handler(FIELD_STRING, client_id)

// Notice about which players are in the game
// Could add some kind of status package here
#define FIELDS_PlayerInformation(field_handler) \
    field_handler(FIELD_STRING, player_name) \
    field_handler(FIELD_UINT8, team) 

// Server responds to a client's 'LEAVE_GAME' request
#define FIELDS_AcknowledgeLeave(field_handler) \
    field_handler(FIELD_STRING, client_id)


MESSAGE_FACTORY_HEADER

#endif