#include <iostream>
#include "event.h"


// In Event.cpp
Event::~Event() {
    // Destructor implementation (even if empty)
}

bool Event::Serialize(Buffer * buffer) {
    switch(this->GetMessageSubType()) {
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
        default:
        return false;
    }
    return false;
}

// Factory function to Deserialize an event
// Returns an event type based on the type 
// of event deserialized
Event* Event::Deserialize(Buffer *buffer) {
    Event *event = NULL;

    switch(GetSubMessageTypeFromHeader(buffer->Peek8())) {
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
    this->SetMessageType(MessageType::EVENT);
    this->SetMessageSubType(PLAYER_JOINS);
    this->player_number = player_number;
    this->player_name = player_name;
}

// Try to deserialize an EventPlayerJoins event
Event * EventPlayerJoins::Deserialize(Buffer *buffer) {
    if(!(buffer->AvailableRead() >= 3)) {
        return NULL;
    }
    // Consume event type header:
    buffer->Read8();
    Uint8 player_number = buffer->Read8(); 
    std::string player_name = buffer->ReadString();
    return new EventPlayerJoins(player_number, player_name.c_str());
}

// Serialize event into buffer. Return false if insufficient space in buffer
bool EventPlayerJoins::Serialize(Buffer * buffer) {
    if(!(buffer->AvailableWrite() >= (this->player_name.length() + 3))) {
        return false;
    }
    buffer->Write8(this->header);
    buffer->Write8(this->player_number);
    buffer->WriteString(this->player_name.c_str());
    return true;
}

EventPlayerLeaves::EventPlayerLeaves(Uint8 player_number, Uint8 reason) {
    this->SetMessageType(MessageType::EVENT);
    this->SetMessageSubType(PLAYER_LEAVES);
    this->player_number = player_number;
    this->reason = reason;
}

// Try to deserialize an EventPlayerLeaves event
Event * EventPlayerLeaves::Deserialize(Buffer *buffer) {
    if(!(buffer->AvailableRead() >= 2)) {
        return NULL;
    }
    // Consume event type header:
    buffer->Read8();
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
    buffer->Write8(this->header);
    buffer->Write8(this->player_number);
    return true;
}

EventTeamWins::EventTeamWins(Uint8 team) {
    this->SetMessageType(MessageType::EVENT);
    this->SetMessageSubType(TEAM_WINS);
    this->team = team;
}

// Try to deserialize an EventTeamWins event
Event * EventTeamWins::Deserialize(Buffer *buffer) {
    if(!(buffer->AvailableRead() >= 2)) {
        return NULL;
    }
    // Consume event type header:
    buffer->Read8();
    return new EventTeamWins(buffer->Read8());
}

// Serialize event into buffer. Return false if insufficient space in buffer
bool EventTeamWins::Serialize(Buffer * buffer) {
    if(!(buffer->AvailableWrite() >= 2)) {
        return false;
    }
    buffer->Write8(this->header);
    buffer->Write8(this->team);
    return true;
}

EventLevelChange::EventLevelChange(const char * level_name, const char *message) {
    this->SetMessageType(MessageType::EVENT);
    this->SetMessageSubType(LEVEL_CHANGE);
    this->level = level_name;
    this->message = message;
}

// Try to deserialize an EventLevelChange event
Event * EventLevelChange::Deserialize(Buffer *buffer) {
    if(!(buffer->AvailableRead() >= 2)) {
        return NULL;
    }
    // Consume event type header:
    buffer->Read8();
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
    buffer->Write8(this->header);
    buffer->WriteString(this->level.c_str());
    buffer->WriteString(this->message.c_str());
    return true;
}

EventServerQuit::EventServerQuit(const char *message) {
    this->SetMessageType(MessageType::EVENT);
    this->SetMessageSubType(SERVER_QUIT);
    this->message = message;
}

// Try to deserialize an EventServerQuit event
Event * EventServerQuit::Deserialize(Buffer *buffer ) {
    if(!(buffer->AvailableRead() >= 2)) {
        return NULL;
    }
    // Consume event type header:
    buffer->Read8();
    std::string message = buffer->ReadString();
    return new EventServerQuit(message.c_str());
}

// Serialize event into buffer. Return false if insufficient space in buffer
bool EventServerQuit::Serialize(Buffer * buffer) {
    if(!(buffer->AvailableWrite() >= ( this->message.length() + 2 ))) {
        return false;
    }
    buffer->Write8(this->header);
    buffer->WriteString(this->message.c_str());
    return true;
}