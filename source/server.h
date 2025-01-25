#ifndef SHIPZSERVER_H
#define SHIPZSERVER_H

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

class Server {
    private:
        Buffer sendbuf;
        SDLNet_Datagram * in;
        SDLNet_Address * ipaddr;
        SDLNet_DatagramSocket * udpsock;
        int error = 0;
        bool ** collisionmap;
        int done = 0;
        int number_of_players = 0;
        SDLNet_Address * my_ip_address;
        std::vector<Event*> events;
        uint runstate;

        std::vector<Player> players;

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
        void SendBuffer(SDLNet_Address * client_address);
        void UpdateBases();
};

#endif
