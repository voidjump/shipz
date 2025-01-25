#ifndef SHIPZSERVER_H
#define SHIPZSERVER_H

#include "types.h"
#include "event.h"
#include "net.h"
#include "team.h"
#include "player.h"
#include "socket.h"

#define SERVER_RUNSTATE_OK 1
#define SERVER_RUNSTATE_FAIL 2
#define SERVER_RUNSTATE_QUIT 3

#define IDLETIMEBEFOREDROP 2000

class Server {
    private:
        int done = 0;
        
        std::string level_name;
        Socket socket;

    public:
        // Start server
        Server(std::string level_name, uint16_t port);
        ~Server();

        void Run();
        void Init();
        bool Load();
        void GameLoop();
};

#endif
