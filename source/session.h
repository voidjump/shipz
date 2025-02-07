#ifndef SHIPZ_SESSION_H
#define SHIPZ_SESSION_H

#include "message_factory.h"

/*
 * Session message: packet authentication
 *
*/
using ShipzSession = uint16_t;
constexpr ShipzSession NO_SHIPZ_SESSION = 0;

#undef BASE_CLASS_NAME
#undef BASE_CLASS_HEADER
#undef MESSAGE_CLASS_LIST
#define BASE_CLASS_NAME Session
#define BASE_CLASS_HEADER SESSION

#define MESSAGE_CLASS_LIST(class_handler) \
    class_handler(RequestSession,       REQUEST_SESSION) \
    class_handler(ProvideSession,       PROVIDE_SESSION) \
    class_handler(TokenData,            TOKEN_DATA)

// Request a session (from client)
#define FIELDS_RequestSession(field_handler) \
    field_handler(FIELD_UINT8, version) \
    field_handler(FIELD_UINT16, port)

// Server provides session id for client session
#define FIELDS_ProvideSession(field_handler) \
    field_handler(FIELD_UINT16, session_id)

// Server provides session id for client session
#define FIELDS_TokenData(field_handler) \
    field_handler(FIELD_UINT16, session_id)

MESSAGE_FACTORY_HEADER

#endif
