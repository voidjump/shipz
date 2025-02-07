#ifndef SHIPZSERVER_H
#define SHIPZSERVER_H

#include <vector>
#include <asio.hpp>
#include "types.h"
#include "event.h"
#include "net.h"
#include "team.h"
#include "player.h"

#define SERVER_RUNSTATE_OK 1
#define SERVER_RUNSTATE_FAIL 2
#define SERVER_RUNSTATE_QUIT 3

#define IDLETIMEBEFOREDROP 2000

int RunServer();
using asio::ip::udp;

class Server {
    private:
        Buffer sendbuf;
        Buffer receive_buffer;
        int error = 0;
        bool ** collisionmap;
        int done = 0;
        int number_of_players = 0;
        SDLNet_Address * my_ip_address;
        std::vector<Event*> events;
        uint runstate;

        Player players[MAXPLAYERS];
        asio::io_context io_context;
        udp::socket * socket;
        udp::endpoint remote_address;

    public:
        // Start server
        Server(const char *);
        ~Server();

        Uint8 CheckVictory();
        void SendEvent(Event *event);
        void Init();
        void Run();
        void GameLoop();
        void LoadLevel();
        void Tick();
        void HandleLeave();
        void HandleUpdate();
        void HandleJoin();
        void HandleStatus();
        void HandleChat();
        void CheckIdlePlayers();
        void UpdatePlayers();
        void SendUpdates();
        // Send buffer to a client
        void SendBuffer(asio::ip::udp::endpoint client_address);
        void UpdateBases();
        void ReceiveUDP();
        void HandlePacket();
};

#endif
