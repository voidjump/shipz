#include <iostream>
#include "event.h"


// In Event.cpp
Event::~Event() {
    // Destructor implementation (even if empty)
}

bool Event::Serialize(Buffer * buffer) {
    switch(event_type) {
        case PLAYER_JOINS:
            return static_cast<EventPlayerJoins*>(this)->Serialize(buffer);
            break;
        case PLAYER_LEAVES:
            return static_cast<EventPlayerLeaves*>(this)->Serialize(buffer);
            break;
        case TEAM_WINS:
            return static_cast<EventTeamWins*>(this)->Serialize(buffer);
            break;
        case LEVEL_CHANGE:
            return static_cast<EventLevelChange*>(this)->Serialize(buffer);
            break;
        case SERVER_QUIT:
            return static_cast<EventServerQuit*>(this)->Deserialize(buffer);
            break;
    }
    return false;
}

// Factory function to Deserialize an event
// Returns an event type based on the type 
// of event deserialized
Event* Event::Deserialize(Buffer *buffer) {
    Event *event;
    // if( buffer->Read8() != SHIPZ_MESSAGE::EVENT ) {
    //     return NULL;
    // }
    std::cout << "team wins?" << std::endl;

    switch(buffer->Read8()) {
        case PLAYER_JOINS:
            event = EventPlayerJoins::Deserialize(buffer);
            break;
        case PLAYER_LEAVES:
            event = EventPlayerLeaves::Deserialize(buffer);
            break;
        case TEAM_WINS:
            event = EventTeamWins::Deserialize(buffer);
            break;
        case LEVEL_CHANGE:
            event = EventLevelChange::Deserialize(buffer);
            break;
        case SERVER_QUIT:
            event = EventServerQuit::Deserialize(buffer);
            break;
        default:
            std::cout << "debug: Unknown event type" << std::endl;
            return NULL;
    }
    return event;
}

EventPlayerJoins::EventPlayerJoins(Uint8 player_number, const char * player_name) {
    this->event_type = PLAYER_JOINS;
    this->player_number = player_number;
    this->player_name = player_name;
}

// Try to deserialize an EventPlayerJoins event
Event * EventPlayerJoins::Deserialize(Buffer *buffer) {
    // Try to read a player number
    if(!(buffer->AvailableRead() >= 2)) {
        return NULL;
    }
    Uint8 player_number = buffer->Read8(); 
    std::string player_name = buffer->ReadString();
    return new EventPlayerJoins(player_number, player_name.c_str());
}

// Serialize event into buffer. Return false if insufficient space in buffer
bool EventPlayerJoins::Serialize(Buffer * buffer) {
    if(!(buffer->AvailableWrite() >= (this->player_name.length() + 3))) {
        return false;
    }
    buffer->Write8(this->event_type);
    buffer->Write8(this->player_number);
    buffer->WriteString(this->player_name.c_str());
    return true;
}

EventPlayerLeaves::EventPlayerLeaves(Uint8 player_number, Uint8 reason) {
    this->event_type = PLAYER_LEAVES;
    this->player_number = player_number;
    this->reason = reason;
}

// Try to deserialize an EventPlayerLeaves event
Event * EventPlayerLeaves::Deserialize(Buffer *buffer) {
    if(!(buffer->AvailableRead() >= 1)) {
        return NULL;
    }
    Uint8 player_number = buffer->Read8(); 
    if(!(buffer->AvailableRead() >= 1)) {
        return NULL;
    }
    Uint8 reason = buffer->Read8(); 
    return new EventPlayerLeaves(player_number, reason);
}

// Serialize event into buffer. Return false if insufficient space in buffer
bool EventPlayerLeaves::Serialize(Buffer * buffer) {
    if(!(buffer->AvailableWrite() >= 2)) {
        return false;
    }
    buffer->Write8(this->event_type);
    buffer->Write8(this->player_number);
    return true;
}

EventTeamWins::EventTeamWins(Uint8 team) {
    this->event_type = TEAM_WINS;
    this->team = team;
}

// Try to deserialize an EventTeamWins event
Event * EventTeamWins::Deserialize(Buffer *buffer) {
    if(!(buffer->AvailableRead() >= 1)) {
        return NULL;
    }
    return new EventTeamWins(buffer->Read8());
}

// Serialize event into buffer. Return false if insufficient space in buffer
bool EventTeamWins::Serialize(Buffer * buffer) {
    if(!(buffer->AvailableWrite() >= 2)) {
        return false;
    }
    buffer->Write8(this->event_type);
    buffer->Write8(this->team);
    return true;
}

EventLevelChange::EventLevelChange(const char * level_name, const char *message) {
    this->event_type = LEVEL_CHANGE;
    this->level = level_name;
    this->message = message;
}

// Try to deserialize an EventLevelChange event
Event * EventLevelChange::Deserialize(Buffer *buffer) {
    if(!(buffer->AvailableRead() >= 1)) {
        return NULL;
    }
    std::string level_name = buffer->ReadString();
    if(!(buffer->AvailableRead() >= 1)) {
        return NULL;
    }
    std::string message = buffer->ReadString();
    return new EventLevelChange(level_name.c_str(), message.c_str());
}
// Serialize event into buffer. Return false if insufficient space in buffer
bool EventLevelChange::Serialize(Buffer * buffer) {
    if(!(buffer->AvailableWrite() >= (this->level.length() + 1 
                                     + this->message.length() + 2))) {
        return false;
    }
    buffer->Write8(this->event_type);
    buffer->WriteString(this->level.c_str());
    buffer->WriteString(this->message.c_str());
    return true;
}

EventServerQuit::EventServerQuit(const char *message) {
    this->event_type = SERVER_QUIT;
    this->message = message;
}

// Try to deserialize an EventServerQuit event
Event * EventServerQuit::Deserialize(Buffer *buffer ) {
    if(!(buffer->AvailableRead() >= 1)) {
        return NULL;
    }
    std::string message = buffer->ReadString();
    return new EventServerQuit(message.c_str());
}

// Serialize event into buffer. Return false if insufficient space in buffer
bool EventServerQuit::Serialize(Buffer * buffer) {
    if(!(buffer->AvailableWrite() >= ( this->message.length() + 2 ))) {
        return false;
    }
    buffer->Write8(this->event_type);
    buffer->WriteString(this->message.c_str());
    return true;
}