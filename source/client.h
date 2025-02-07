#ifndef SHIPZCLIENT_H
#define SHIPZCLIENT_H
#include <asio.hpp>
#include "types.h"
#include "net.h"
#include "player.h"
using asio::ip::udp;

class Client {
    private:
        Buffer send_buffer; // transmission buffer
        Buffer receive_buffer;
        Buffer type_buffer;
        const bool* keys;
        asio::io_context io_context;

        const char* server_address; // The address we want to connect to
        const char* name; // The player's name

        udp::socket * socket;
        udp::endpoint server_endpoint;
	    asio::ip::udp::endpoint client_endpoint;
        bool packet_ready;

        int error = 0;
        int attempts = 0;
        int done = 0;
        int my_player_nr;
        int number_of_players = 1;
        int screenshotcounter = 0;
        int charstyped = 0;
        
        bool notreadytocontinue = 1;	
        bool input_given = 0;  // used to decide whether to send a package to the server or not

        Player * self;
        Player players[MAXPLAYERS];

        char chat1[MAXCHATCHARS], chat2[MAXCHATCHARS], chat3[MAXCHATCHARS];
    
    public:
        Client(const char *, const char * );
        ~Client(); // Destructor

        // Run the game
        void Run(); 

        // Initialize some state
        void Init(); 

        // Resolve a hostname
        void ResolveHostname(const char *);

        // Create a socket
        void CreateUDPSocket();

        // Send buffer to server
        void SendBuffer();

        // Connect to a server
        // This should probably return some levelinfo
        void Connect(const char *);

        // Leave the game
        void Leave();

        // Load levelinfo
        void Load();

        // Draw game
        void Draw();

        // Run the game loop
        void GameLoop();

        // Join a server
        bool Join();

        // Handle keyboard inputs
        void HandleInputs();

        // Take a screeshot
        void TakeScreenShot();

        // Send a chatline
        void SendChatLine();

        // Send an periodic update to the server
        void SendUpdate();

        // Fail with an error
        void FailErr(const char *);

        // Wait for a packet for a while
        bool WaitForPacket();

        // Wait for a packet without blocking
        bool ReceivedPacket();

        // Update timers
        void Tick();

        // Update other players
        void UpdatePlayers();

        // Packet handlers:
        void HandleKicked();
        void HandleUpdate();
        void HandleChat();
        void HandlePlayerJoins();
        void HandlePlayerLeaves();
        void HandleEvent();
        void ReceiveUDP();
        void HandlePacket();
};

#endif
