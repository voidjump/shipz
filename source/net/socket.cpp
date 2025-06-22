#include "net/socket.h"

#include <asio.hpp>
#include <sstream>

#include "utils/log.h"
using asio::ip::udp;

/// @brief Resolve a hostname
/// @param connect_address string representation of the hostname
/// @return resolved endpoint
udp::endpoint Socket::ResolveHostname(asio::io_context &context,
                                      const char *connect_address,
                                      Uint16 port) {
    udp::resolver resolver(context);
    char buffer[10];

    // TODO implement timeout
    snprintf(buffer, 9, "%i", PORT_SERVER);
    log::info("resolving hostname ", connect_address, " ...");
    udp::endpoint resolve_endpoint =
        *resolver.resolve(udp::v4(), connect_address, buffer).begin();
    resolve_endpoint.port(port);
    log::info("resolved ", resolve_endpoint.address().to_string());
    return resolve_endpoint;
}

// void Client::CreateUDPSocket() {
//     this->socket = new udp::socket(io_context, udp::endpoint(udp::v4(),
//     PORT_CLIENT));
// 	// socket->open(udp::v4());
// }

/// @brief Send a buffer-like object as datagram
/// @param buffer the buffer to send
/// @param address the remote address
/// @param port the remote port
/// @return whether succesful
bool Socket::Send(Buffer &buffer, udp::endpoint endpoint) {
    if (!this->socket || !this->socket->is_open()) {
        log::error("cannot send, socket is not initialized or open");
        return false;
    }
    if (endpoint.address().is_unspecified() || endpoint.port() == 0) {
        log::error("cannot send, empty address");
        return false;
    }

    if (buffer.length == 0) {
        log::error("cannot send, empty buffer");
        return false;
    }

    // log::debug("sending buffer:", buffer.AsHexString());
    try {
        this->socket->send_to(asio::buffer(buffer.data, buffer.length),
                              endpoint);
    } catch (const std::exception &e) {
        log::error(e.what());
        return false;
    }

    return true;
}

/// @brief check if there are any available packets, adding them to queue
/// @return whether any packets are waiting in queue
bool Socket::Poll() {
    asio::io_context& ctx = static_cast<asio::io_context&>(this->socket->get_executor().context());
    ctx.poll();
    return !in_queue.Empty();
}

void Socket::ReceiveUDP() {
    this->receive_buffer.Clear();

    static udp::endpoint remote_endpoint;
    this->socket->async_receive_from(
        asio::buffer(this->receive_buffer.data, MAXBUFSIZE), remote_endpoint,
        [this](std::error_code ec, std::size_t bytes_received) {
            if (!ec) {
                std::cout << "received packet!" << std::endl;
                receive_buffer.OutputDebug();
                this->receive_buffer.Seek(0);
                this->receive_buffer.length = bytes_received;

                // import packet to new packet
                Packet pack;
                if (!pack.ImportBytes(receive_buffer.data,
                                      receive_buffer.length)) {
                    log::error("failed to import packet to buffer");
                } else {
                    pack.ReadSession();
                    pack.origin = remote_endpoint;
                }
                // log::debug("received buffer:", pack.AsHexString());
                in_queue.Push(std::make_unique<Packet>(std::move(pack)));

                ReceiveUDP();
            }
        });
}

/// @brief Construct a socket
/// @param listen_port the port to listen on
Socket::Socket(asio::io_context &context, Uint16 listen_port) {
    this->socket =
        new udp::socket(context, udp::endpoint(udp::v4(), listen_port));
    this->listen_port = listen_port;
    log::debug("started listening on port ", listen_port);
}