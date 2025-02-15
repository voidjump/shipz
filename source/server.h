#ifndef SHIPZSERVER_H
#define SHIPZSERVER_H

#include "chat.h"
#include "event.h"
#include "message_handler.h"
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
#define MAX_SESSION_AGE 10 * 1000

class Server {
   private:
    bool done = false;

    uint max_clients;
    std::string level_name;
    Socket socket;
    MessageHandler handler;
    ChatConsole console;
    std::vector<ShipzSession*> active_sessions;

   public:
    // Start server
    Server(std::string level_name, uint16_t port, uint max_clients);
    ~Server();
    void SetupCallbacks();

    void HandleUnknownMessage(MessagePtr msg, ShipzSession* session);
    void HandleJoin(MessagePtr msg, ShipzSession* session);
    void HandleInfo(MessagePtr msg, ShipzSession* session);
    void HandleCreateSession(MessagePtr msg, ShipzSession* session);

    void PurgeStaleSessions();
    ShipzSession* CreateSessionForClient(SDLNet_Address *addr, uint16_t port);

    void Run();
    void Init();
    bool Load();
    void GameLoop();
    void HandleInboundMessages();
    void SendOutboundMessages();
};

#endif
