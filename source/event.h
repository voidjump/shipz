#ifndef SHIPZ_EVENT_H
#define SHIPZ_EVENT_H

#include <SDL3/SDL.h>
#include <string>

#include "types.h"
#include "net.h"
#include "message.h"

enum EVENT_TYPE {
    PLAYER_JOINS,
    PLAYER_LEAVES,
    TEAM_WINS,
    LEVEL_CHANGE, 
    SERVER_QUIT
};

// A game event
class Event : public Message {
    public:
        virtual ~Event();
        // Serialize event into a buffer - should return false if buffer too small
        bool Serialize(Buffer *);
        // Deserialize buffer into a event
        static Event* Deserialize(Buffer *);
};

class EventPlayerJoins : public Event {
    public:
        Uint8 player_number;
        std::string player_name;
        bool Serialize(Buffer *);

        // Constructor
        EventPlayerJoins(Uint8, const char *);
        static Event * Deserialize(Buffer *buffer);
};

class EventPlayerLeaves : public Event {
    public:
        enum LEAVE_REASON {
            QUIT,
            TIMEOUT,
            KICKED
        };
        Uint8 player_number;
        Uint8 reason;
        bool Serialize(Buffer *buffer);

        EventPlayerLeaves(Uint8 number, Uint8 leave_reason);
        static Event * Deserialize(Buffer *buffer);
};

class EventTeamWins : public Event {
    public:
        Uint8 team;
        bool Serialize(Buffer *);
        EventTeamWins(Uint8);
        static Event * Deserialize(Buffer *buffer);
};

class EventLevelChange : public Event {
    public:
        std::string level;
        std::string message;
        bool Serialize(Buffer *);
        EventLevelChange(const char *, const char *);
        static Event * Deserialize(Buffer *buffer);
};

class EventServerQuit : public Event {
    public:
        std::string message;
        EventServerQuit(const char *);
        bool Serialize(Buffer *);
        static Event * Deserialize(Buffer *buffer);
};

#endif