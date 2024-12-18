#include "ServerSocket.h"
#include "PlayerDisplay.hpp"
#include <algorithm>
#include <cstdio>

// Constructor: Initializes Winsock and sets up the server socket
ServerSocket::ServerSocket(int _port, PlayerDisplay* _playerDisplay)
    : m_socket(INVALID_SOCKET), playerDisplay(_playerDisplay)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }

    addrinfo hints = { 0 };
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    addrinfo* result = NULL;

    if (getaddrinfo(NULL, std::to_string(_port).c_str(), &hints, &result) != 0) {
        throw std::runtime_error("Failed to resolve server address or port");
    }

    m_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (m_socket == INVALID_SOCKET) {
        freeaddrinfo(result);
        throw std::runtime_error("Failed to create socket");
    }

    if (bind(m_socket, result->ai_addr, result->ai_addrlen) == SOCKET_ERROR) {
        freeaddrinfo(result);
        throw std::runtime_error("Failed to bind socket");
    }

    freeaddrinfo(result);

    if (listen(m_socket, SOMAXCONN) == SOCKET_ERROR) {
        throw std::runtime_error("Failed to listen on socket");
    }

    u_long mode = 1; // Set non-blocking mode
    if (ioctlsocket(m_socket, FIONBIO, &mode) == SOCKET_ERROR) {
        throw std::runtime_error("Failed to set non-blocking mode");
    }
}

// Destructor: Cleans up the socket
ServerSocket::~ServerSocket()
{
    closeAllClients();
    closesocket(m_socket);
    WSACleanup();
}

// Accept a new client connection
std::shared_ptr<ClientSocket> ServerSocket::accept()
{
    SOCKET socket = ::accept(m_socket, NULL, NULL);
    if (socket == INVALID_SOCKET) {
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
            printf("Failed to accept connection: %d\n", WSAGetLastError());
        }
        return nullptr;
    }

    return std::make_shared<ClientSocket>(socket);
}

// Broadcast a message to all clients
void ServerSocket::broadcastMessage(const std::string& message)
{
    for (const auto& client : clients) {
        client->send(message);
    }
}

// Gracefully close all clients
void ServerSocket::closeAllClients()
{
    for (auto& client : clients) {
        client->send("[SERVER]: Server is shutting down.");
        client->closed();
    }
    clients.clear();
}

// Handles all client connections
void ServerSocket::handleClientConnections() {
    // Accept new client connections
    std::shared_ptr<ClientSocket> client = accept();
    if (client) {
        printf("Client Connected!\n");

        // Receive username upon connection
        std::string username;
        if (client->receive(username)) {
            client->setUsername(username);
            printf("Username received: %s\n", username.c_str());
            playerDisplay->addPlayer(username); // Add the player to the display

            // Announce new connection to all clients
            broadcastMessage("[SERVER]: " + username + " has joined the server.");
        }

        // Add the new client to the list
        clients.push_back(client);
    }

    // Process messages from connected clients and handle disconnections
    clients.erase(std::remove_if(clients.begin(), clients.end(),
        [&](const std::shared_ptr<ClientSocket>& c) {
            std::string message;

            if (c->receive(message)) {
                broadcastMessage(c->getUsername() + ": " + message);
                return false;
            }

            if (c->closed()) {
                // When the client disconnects, remove them from the display
                playerDisplay->removePlayer(c->getUsername());

                // Announce disconnection to all clients
                broadcastMessage("[SERVER]: " + c->getUsername() + " has disconnected.");
                return true;
            }

            return false;
        }), clients.end());
}
