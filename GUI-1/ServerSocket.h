#pragma once
#include <winsock2.h>
#include <memory>
#include <vector>
#include "ClientSocket.h"

// Forward declaration of ClientSocket
struct ClientSocket;

// ServerSocket class definition
struct ServerSocket {
    ServerSocket(int _port); // Constructor
    ~ServerSocket(); // Destructor
    std::shared_ptr<ClientSocket> accept(); // Accept a new client connection
    std::vector<std::shared_ptr<ClientSocket>>& getClients(); // Get the list of connected clients
    void addClient(std::shared_ptr<ClientSocket> client); // Add a new client to the list
    void removeClient(std::shared_ptr<ClientSocket> client); // Remove a client from the list
    void broadcastMessage(const std::string& message);
    void broadcastUserList();

    void setUsername(const std::string& username) {
        this->username = username; // Assuming `username` is a member variable.
    }

private:
    SOCKET m_socket; // Socket handle for the server
    std::vector<std::shared_ptr<ClientSocket>> m_clients; // List of connected clients
    ServerSocket(const ServerSocket& _copy); // Copy constructor (disabled)
    ServerSocket& operator=(const ServerSocket& _assign); // Assignment operator (disabled)
    std::string username;
};