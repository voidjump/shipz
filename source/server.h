#ifndef SHIPZSERVER_H
#define SHIPZSERVER_H

#include "types.h"
#include "net.h"

#define SERVER_RUNSTATE_OK 1
#define SERVER_RUNSTATE_FAIL 2
#define SERVER_RUNSTATE_QUIT 3

int RunServer();

class Server {
    private:
        Buffer sendbuf;
        SDLNet_Datagram * in;
        SDLNet_Address * ipaddr;
        SDLNet_DatagramSocket * udpsock;
        int error = 0;
        Player players[MAXPLAYERS];
        bool ** collisionmap;
        int done = 0;
        int number_of_players = 0;
        SDLNet_Address * my_ip_address;

        uint runstate;

    public:
        // Start server
        Server(const char *);
        ~Server();

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
};

#endif
