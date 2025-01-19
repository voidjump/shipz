#include "chat.h"

ChatLine::ChatLine(std::string line, Uint8 client_id, Uint8 team) {
    this->line = line;
    this->client_id = client_id;
    this->team = team;
}

// Add a line to the console
void ChatConsole::AddLine(std::string line, Uint8 send_id, Uint8 team_id) {
    lines.push_back(ChatLine(line, send_id, team_id));
}

// Add a line to the console from a chat event message
void ChatConsole::AddFromMessage(EventChat *event) {
    AddLine(event->message, event->client_id, event->team);
}

// Clear console
void ChatConsole::Clear() {
    lines.clear();
}