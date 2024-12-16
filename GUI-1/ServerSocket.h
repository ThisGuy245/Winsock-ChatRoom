#ifndef SERVERSOCKET_H
#define SERVERSOCKET_H

#include <WinSock2.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include "ClientSocket.h"

class ServerSocket {
public:
    // Constructor & Destructor
    ServerSocket(int port);
    ~ServerSocket();

    // Accept a new client
    std::shared_ptr<ClientSocket> accept();

    // Constantly handle client connections (to be run on a timer)
    void handleClientConnections();

    // Broadcast a message to all clients except one
    void broadcastMessage(const std::string& message, std::shared_ptr<ClientSocket> excludeClient = nullptr);

private:
    SOCKET m_socket;                                   // Server socket
    std::vector<std::shared_ptr<ClientSocket>> clients; // List of connected clients
    std::unordered_map<std::shared_ptr<ClientSocket>, std::string> clientUsernames; // Map of clients to their usernames
};

#endif // SERVERSOCKET_H
