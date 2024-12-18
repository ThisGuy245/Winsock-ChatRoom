#ifndef SERVERSOCKET_H
#define SERVERSOCKET_H

#include <winsock2.h>
#include <memory>
#include <vector>
#include <string>
#include "ClientSocket.h"
#include "PlayerDisplay.hpp"  // Include the header where PlayerDisplay is declared

class ServerSocket
{
public:
    ServerSocket(int _port, PlayerDisplay* _playerDisplay);
    ~ServerSocket();

    // Accepts a new client connection and returns a shared pointer to it
    std::shared_ptr<ClientSocket> accept();

    void closeAllClients();
    void broadcastMessage(const std::string& message);

    // Handles all client connections and incoming messages
    void handleClientConnections();

    PlayerDisplay* playerDisplay;

private:
    SOCKET m_socket;  // Main server socket
    std::vector<std::shared_ptr<ClientSocket>> clients; // Stores connected clients

    // Disable copying and assignment
    ServerSocket(const ServerSocket& _copy) = delete;
    ServerSocket& operator=(const ServerSocket& _assign) = delete;
};

#endif // SERVERSOCKET_H
