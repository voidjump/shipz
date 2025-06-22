#pragma once
#include <SDL3_net/SDL_net.h>

#include <queue>
#include <string>
#include <asio.hpp>

#include "net/packet.h"
using asio::ip::udp;

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
    Uint16 listen_port;
    PacketQueue in_queue;
    udp::socket * socket;
    Buffer receive_buffer;

   public:
    Socket(asio::io_context &context, Uint16 listen_port);

    void ReceiveUDP();

    // Resolve a hostname
    static udp::endpoint ResolveHostname(asio::io_context &context, const char *connect_address, Uint16 port );

    // Send buffer-like obj to a remote address and port
    bool Send(Buffer &buffer, udp::endpoint endpoint);

    // Get packet off queue
    std::unique_ptr<Packet> GetPacket() { return in_queue.Pop(); }

    // check if there are any available packets, adding them to queue
    bool Poll();
};