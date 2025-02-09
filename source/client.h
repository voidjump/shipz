#ifndef SHIPZCLIENT_H
#define SHIPZCLIENT_H
#include <sstream>
#include "net.h"
#include "packet.h"
#include "player.h"
#include "session.h"
#include "socket.h"
#include "types.h"
#include "chat.h"

class Client {
   private:
    // initialized values
    std::string name;             // The player's name
    std::string server_hostname;  // The hostname we want to connect to
    Uint16 server_port;           // The port to connect to
    Uint16 listen_port;           // The port this listens on

    // networking state
    ShipzSessionID session;
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
    // This should probably return some levelinfo
    ShipzSessionID Connect();

    // Leave the game
    void Leave();

    // Load levelinfo
    bool Load();

    // Draw game
    void Draw();

    // Run the game loop
    void GameLoop();

    // Join a server
    bool Join();

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

    // Set up message handling callbacks
    void SetupCallbacks();
    
    // Packet handlers
    void HandleKicked(std::shared_ptr<Message> msg);
    void HandleChat(std::shared_ptr<Message> msg);
    void HandlePlayerJoins(std::shared_ptr<Message> msg);
    void HandlePlayerLeaves(std::shared_ptr<Message> msg);
    void HandleUnknownMessage(std::shared_ptr<Message> msg);
    void HandleObjectSpawn(std::shared_ptr<Message> msg);
    void HandleObjectUpdate(std::shared_ptr<Message> msg);
    void HandleObjectDestroy(std::shared_ptr<Message> msg);
    void HandlePlayerState(std::shared_ptr<Message> msg);
    void HandleTeamStates(std::shared_ptr<Message> msg);
};

#endif
