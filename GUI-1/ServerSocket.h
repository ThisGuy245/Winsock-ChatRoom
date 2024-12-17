#ifndef SERVERSOCKET_H
#define SERVERSOCKET_H

#include <winsock2.h>
#include <memory>
#include <vector>
#include "ClientSocket.h"
#include "PlayerDisplay.hpp"  // Include the header where PlayerDisplay is declared

// Declaration of globalPlayerDisplay to be used across multiple files
extern PlayerDisplay* globalPlayerDisplay;

struct ServerSocket
{
    ServerSocket(int _port);
    ~ServerSocket();

    std::shared_ptr<ClientSocket> accept();
    void handleClientConnections();

private:
    SOCKET m_socket;
    bool m_closed;
    ServerSocket(const ServerSocket& _copy);
    ServerSocket& operator=(const ServerSocket& _assign);
    bool receive(std::string& _message);
    std::vector<std::shared_ptr<ClientSocket>> clients; // stores connected clients
};

#endif // SERVERSOCKET_H
