#ifndef SHIPZ_REQUEST_H
#define SHIPZ_REQUEST_H

#include "message_factory.h"

/* 
 * Request message: A message that requires a response
 *
 * Requests are reliable messages that expect a single Response
 */

#undef BASE_CLASS_NAME
#undef BASE_CLASS_HEADER
#undef MESSAGE_CLASS_LIST
#define BASE_CLASS_NAME Request 
#define BASE_CLASS_HEADER REQUEST

#define MESSAGE_CLASS_LIST(class_handler) \
    class_handler(GetServerInfo,    GET_SERVER_INFO) \
    class_handler(JoinGame,         JOIN_GAME) \
    class_handler(LeaveGame,        LEAVE_GAME)

// Request server information
#define FIELDS_GetServerInfo(field_handler) \
    field_handler(FIELD_STRING, client_id) \
    field_handler(FIELD_UINT8, version)

// Request to join the game
#define FIELDS_JoinGame(field_handler) \
    field_handler(FIELD_STRING, player_name)

// Notify that we are quitting
#define FIELDS_LeaveGame(field_handler) \
    field_handler(FIELD_UINT8, client_id) 

MESSAGE_FACTORY_HEADER

#endif