#include <unordered_set>
#include "session.h"

std::unordered_set<ShipzSessionID> ShipzSession::active_ids;

MESSAGE_FACTORY_IMPLEMENTATION