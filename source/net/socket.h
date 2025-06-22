#pragma once
#include <SDL3_net/SDL_net.h>

#include <queue>
#include <string>

#include "net/packet.h"

// shared Socket for client and server
// Wraps SDL socket

class PacketQueue {
   private:
    std::queue<std::unique_ptr<Packet>> queue;

   public:
    // Whether the queue is empty
    inline bool Empty() { return queue.empty(); }

    // The amount of Packets in the queue
    inline size_t Size() { return queue.size(); }

    // Pop Packet off queue
    inline std::unique_ptr<Packet> Pop() {
        if (queue.empty()) return nullptr;
        auto packet = std::move(queue.front());
        queue.pop();
        return packet;
    }

    // Add Packet to queue
    inline void Push(std::unique_ptr<Packet> packet) {
        queue.push(std::move(packet));
    }
};

class Socket {
   private:
    SDLNet_DatagramSocket *udpsock;
    Uint16 listen_port;
    PacketQueue in_queue;

   public:
    Socket(Uint16 listen_port);

    // Resolve a hostname
    static bool ResolveHostname(const char *connect_address,
                                SDLNet_Address **resolve_target,
                                uint timeout = 5);

    // Send buffer-like obj to a remote address and port
    bool Send(Buffer &buffer, SDLNet_Address *address, Uint16 port);

    // Get packet off queue
    std::unique_ptr<Packet> GetPacket() { return in_queue.Pop(); }

    // check if there are any available packets, adding them to queue
    bool Poll();
};