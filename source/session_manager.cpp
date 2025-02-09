#include "session_manager.h"

std::map<SDLNet_Address *, ShipzSession *> ShipzSessionManager::active_sessions;
std::map<ShipzSessionID, ShipzSession *> ShipzSessionManager::by_id_map;
std::map<ShipzSessionID, SessionMessageManager*> ShipzSessionManager::message_managers;