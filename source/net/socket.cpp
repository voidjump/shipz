#include "socket.h"

#include <sstream>

#include "log.h"

/// @brief Resolve a hostname
/// @param connect_address string representation of the hostname
/// @param resolve_target ip address to populate
/// @param timeout max seconds to take
/// @return whether resolved successfully
bool Socket::ResolveHostname(const char *connect_address,
                             SDLNet_Address **resolve_target, uint timeout) {
    *resolve_target = SDLNet_ResolveHostname(connect_address);
    if (resolve_target == NULL) {
        goto fail;
    }

    // wait for the hostname to resolve:
    log::info("resolving hostname ", connect_address, " ...");
    while (SDLNet_GetAddressStatus(*resolve_target) == 0) {
        if (timeout == 0) {
            goto fail;
        }
        log::info(".");
        SDL_Delay(1000);
        timeout--;
    }
    if (SDLNet_GetAddressStatus(*resolve_target) != 1) {
        goto fail;
    }

    log::info("resolved server address: ",
              SDLNet_GetAddressString(*resolve_target));
    return true;

fail:
    log::error("could not resolve server address", connect_address);
    return false;
}

/// @brief Send a buffer-like object as datagram
/// @param buffer the buffer to send
/// @param address the remote address
/// @param port the remote port
/// @return whether succesful
bool Socket::Send(Buffer &buffer, SDLNet_Address *address, Uint16 port) {
    if (!this->udpsock) {
        log::error("cannot send, socket is not initialized");
        return false;
    }
    if (!address) {
        log::error("cannot send, null address");
        return false;
    }
    if (buffer.length == 0) {
        log::error("cannot send, empty buffer");
        return false;
    }
    // log::debug("sending buffer:", buffer.AsHexString());
    if (!SDLNet_SendDatagram(this->udpsock, address, port, (void *)buffer.data,
                             buffer.length)) {
        log::error("can't send buffer to address: ",
                   SDLNet_GetAddressString(address), port);
        return false;
    }
    return true;
}

/// @brief check if there are any available packets, adding them to queue
/// @return whether any packets are waiting in queue
bool Socket::Poll() {
    bool result = false;
    SDLNet_Datagram * recv;

    if (!this->udpsock) {
        log::error("socket is not initialized!");
        return false;
    }
    if (!SDLNet_ReceiveDatagram(udpsock, &recv) || recv == NULL) {
        // did not receive packet or it is invalid
        return false;
    }

    if (recv->buflen == 0) {
        log::debug("received packet is empty");
        SDLNet_DestroyDatagram(recv);
        return false;
    }

    // import packet to new packet
    Packet pack;
    if (!pack.ImportBytes(recv->buf, recv->buflen)) {
        log::error("failed to import packet to buffer");
        result = false;
    } else {
        pack.ReadSession();
        pack.origin = recv->addr;
        SDLNet_RefAddress(recv->addr);
        result = true;
    }
    // log::debug("received buffer:", pack.AsHexString());
    in_queue.Push(std::make_unique<Packet>(std::move(pack)));

    SDLNet_DestroyDatagram(recv);
    return !in_queue.Empty();
}

/// @brief Construct a socket
/// @param listen_port the port to listen on
Socket::Socket(Uint16 listen_port) {
    this->listen_port = listen_port;
    this->udpsock = SDLNet_CreateDatagramSocket(NULL, this->listen_port);

    if (this->udpsock == NULL) {
        SDLNet_DestroyDatagramSocket(udpsock);
        throw new std::runtime_error("failed to open socket");
    }
	log::debug("started listening on port ", listen_port);
}
