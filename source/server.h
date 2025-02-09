#ifndef SHIPZSERVER_H
#define SHIPZSERVER_H

#include "chat.h"
#include "event.h"
#include "net.h"
#include "packet.h"
#include "player.h"
#include "session.h"
#include "socket.h"
#include "team.h"
#include "types.h"

#define SERVER_RUNSTATE_OK 1
#define SERVER_RUNSTATE_FAIL 2
#define SERVER_RUNSTATE_QUIT 3

#define IDLETIMEBEFOREDROP 2000


class Server {
   private:
    bool done = false;

    uint max_clients;
    std::string level_name;
    Socket socket;
    MessageHandler handler;
    ChatConsole console;

   public:
    // Start server
    Server(std::string level_name, uint16_t port, uint max_clients);
    ~Server();
    void HandleUnknownMessage(std::shared_ptr<Message> msg);
    void SetupCallbacks();
    void HandleJoin(std::shared_ptr<Message> msg);
    void HandleInfo(std::shared_ptr<Message> msg);
    void CreateSession(std::shared_ptr<Message> msg);

    void Run();
    void Init();
    bool Load();
    void GameLoop();
};

#endif
