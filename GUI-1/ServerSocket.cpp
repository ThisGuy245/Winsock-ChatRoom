#include <WS2tcpip.h>
#include <stdexcept>
#include <string>
#include "ServerSocket.h"
#include "ClientSocket.h"
#include "PlayerDisplay.hpp"

// Global player display for managing connected players
PlayerDisplay* globalPlayerDisplay = nullptr;

// Constructor: Initializes Winsock and sets up the server socket
ServerSocket::ServerSocket(int _port) : m_socket(INVALID_SOCKET)
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

    // Resolve server address and port
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

    // Set the socket to non-blocking mode
    u_long mode = 1;
    if (ioctlsocket(m_socket, FIONBIO, &mode) == SOCKET_ERROR) {
        throw std::runtime_error("Failed to set non-blocking mode");
    }
}

// Destructor: Cleans up the socket
ServerSocket::~ServerSocket()
{
    closesocket(m_socket);
    WSACleanup();
}

// Accept a new client connection
std::shared_ptr<ClientSocket> ServerSocket::accept()
{
    SOCKET socket = ::accept(m_socket, NULL, NULL);
    if (socket == INVALID_SOCKET) {
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
            throw std::runtime_error("Failed to accept socket");
        }
        return nullptr;
    }

    std::shared_ptr<ClientSocket> client = std::make_shared<ClientSocket>(socket);
    return client;
}

// Handles all client connections: new connections, incoming messages, and disconnections
void ServerSocket::handleClientConnections()
{
    // Accept new client connections
    std::shared_ptr<ClientSocket> client = accept();
    if (client) {
        printf("Client Connected!\n");

        // Request and set username
        std::string username;
        if (client->receive(username)) {
            client->setUsername(username);
            printf("Username received: %s\n", username.c_str());

            if (globalPlayerDisplay) {
                globalPlayerDisplay->addPlayer(username);
            }

            // Broadcast join message
            std::string joinMessage = "[SERVER]: " + username + " has joined the server";
            for (const auto& c : clients) {
                c->send(joinMessage);
            }
        }
        else {
            printf("Failed to receive username from client.\n");
        }

        clients.push_back(client);
    }

    // Process messages from connected clients
    for (size_t i = 0; i < clients.size(); ++i) {
        std::string message;

        if (clients[i]->receive(message)) {
            const std::string& username = clients[i]->getUsername();

            // Check for a username change command
            if (message.rfind("/change_username", 0) == 0) {
                std::string newUsername = message.substr(17);
                if (!newUsername.empty()) {
                    std::string oldUsername = username;
                    clients[i]->setUsername(newUsername);

                    // Notify all clients about the change
                    std::string notification = "[SERVER]: " + oldUsername + " is now known as " + newUsername;
                    printf("%s\n", notification.c_str());
                    for (const auto& c : clients) {
                        c->send(notification);
                    }

                    if (globalPlayerDisplay) {
                        globalPlayerDisplay->removePlayer(oldUsername);
                        globalPlayerDisplay->addPlayer(newUsername);
                    }
                }
            }
            else {
                // Broadcast regular messages
                printf("Message received from %s: %s\n", username.c_str(), message.c_str());
                std::string broadcastMessage = username + ": " + message;
                for (size_t j = 0; j < clients.size(); ++j) {
                    if (i != j) {
                        clients[j]->send(broadcastMessage);
                    }
                }
            }
        }
        else if (clients[i]->closed()) {
            // Handle client disconnection
            const std::string& username = clients[i]->getUsername();
            printf("%s has disconnected\n", username.c_str());
            std::string disconnectMessage = "[SERVER]: " + username + " has disconnected";

            if (globalPlayerDisplay) {
                globalPlayerDisplay->removePlayer(username);
            }

            for (size_t j = 0; j < clients.size(); ++j) {
                if (i != j) {
                    clients[j]->send(disconnectMessage);
                }
            }

            clients.erase(clients.begin() + i);
            i--;
        }
    }
}
