#ifndef SHIPZ_CHAT_H
#define SHIPZ_CHAT_H
#include <iostream>
#include <SDL3/SDL.h>
#include "event.h"

#define MAX_CHAT_LENGTH 79

class ChatLine {
    public:
        std::string line; /* What is being said */
        Uint8 client_id; /* The sender */
        Uint8 team; /* The intended audience */
    
    ChatLine(std::string line, Uint8 client_id, Uint8 team);
};

class ChatConsole {
    private:
        std::vector<ChatLine> lines;
        uint8_t height;

    public:
        void SetHeight(uint8_t height);
        void AddLine(std::string line, Uint8 send_id, Uint8 team_id);
        void AddFromMessage(EventChat *event);
        void Draw();
        void Clear();
};

#endif
