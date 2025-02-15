#ifndef SHIPZCLIENT_H
#define SHIPZCLIENT_H
#include <sstream>
#include "message_handler.h"
#include "net.h"
#include "packet.h"
#include "player.h"
#include "session.h"
#include "socket.h"
#include "types.h"
#include "chat.h"

enum ClientState {
    S_ERROR, 
    S_INITIAL,
    S_REQUEST_INFO,
    S_AWAITING_INFO,
    S_RECEIVED_INFO,
    S_JOINING_SERVER,
    S_AWAITING_JOIN,
    S_SERVER_FULL,
    S_ACCEPTED,
    S_DENIED,
    S_VERSION_MISMATCH,
    S_TIMEOUT
};

class Client {
   private:
    // initialized values
    std::string name;             // The player's name
    std::string server_hostname;  // The hostname we want to connect to
    Uint16 server_port;           // The port to connect to
    Uint16 listen_port;           // The port this listens on

    // networking state
    ShipzSession *session;
    SDLNet_Address* server_address;  // The server
    Socket socket;
    MessageHandler handler;

    // Game related
    Player* self;
    std::vector<Player*> players;
    std::vector<SyncObjectSpawn*> bullets_shot;
    Uint16 client_id;
    bool done;
    ChatConsole console;

    // client state
    int state;

    // UI
    const bool* keys;
    std::string type_buffer;

   public:
    Client(const char* server_address, const char* player_name, Uint16 listen_port, Uint16 server_port);
    ~Client();  // Destructor

    // Run the game
    void Run();

    // Initialize client
    void Init();

    // Connect to a server
    ShipzSession* Connect();

    // Leave the game
    void Leave();

    // Load levelinfo
    bool Load();

    // Draw game
    void Draw();

    // Run the game loop
    void GameLoop();

    // Join a server
    bool JoinLoop();

    // Handle keyboard inputs
    void HandleInputs();

    // Send a chatline
    void SendChatLine();

    // Shoot a bullet
    void Shoot();

    // Send an periodic update to the server
    void SendUpdate();

    // Update timers
    void Tick();

    // Update other players
    void UpdatePlayers();

    // Remove a player from the game
    void RemovePlayer(Uint16 id, std::string reason);

    // Add a player to the game
    void AddPlayer(Uint16 id, std::string player_name, Uint8 team);

    // Set up message handling callbacks (gameloop)
    void SetupCallbacks();

    // Set up message handling callbacks (join)
    void SetupJoinCallsbacks();
    
    // Packet handlers
    void HandleKicked(MessagePtr msg, ShipzSession* session);
    void HandleChat(MessagePtr msg, ShipzSession* session);
    void HandlePlayerJoins(MessagePtr msg, ShipzSession* session);
    void HandlePlayerLeaves(MessagePtr msg, ShipzSession* session);
    void HandleUnknownMessage(MessagePtr msg, ShipzSession* session);
    void HandleObjectSpawn(MessagePtr msg, ShipzSession* session);
    void HandleObjectUpdate(MessagePtr msg, ShipzSession* session);
    void HandleObjectDestroy(MessagePtr msg, ShipzSession* session);
    void HandlePlayerState(MessagePtr msg, ShipzSession* session);
    void HandleTeamStates(MessagePtr msg, ShipzSession*session);
    void HandleServerInfo(MessagePtr msg, ShipzSession* session);
    void HandleAcceptJoin(MessagePtr msg, ShipzSession* session);
    void HandleDenyJoin(MessagePtr msg, ShipzSession* session);
    void HandlePlayerInfo(MessagePtr msg, ShipzSession* session);
};

#endif
