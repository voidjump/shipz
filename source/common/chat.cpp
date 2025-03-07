#include "chat.h"
#include "font.h"
#include "assets.h"

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

// Set the height of the console
void ChatConsole::SetHeight(uint8_t height) {
    this->height = height;
}

// Clear console
void ChatConsole::Clear() {
    lines.clear();
}

// Iterate over last `height` lines in the console, and draw them
void ChatConsole::Draw() {
    int y = 5, lines_done=0;
    for (auto it = lines.rbegin(); it != lines.rend(); ++it) {
        DrawFont(sansbold, it->line.c_str(), 5, y, FONT_COLOR::WHITE);
        y += 11; 
        lines_done++;
        if (lines_done >= height) {
            return;
        }
    }
}
    