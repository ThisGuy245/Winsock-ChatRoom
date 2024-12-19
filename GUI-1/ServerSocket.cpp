#include "ServerSocket.h"
#include "PlayerDisplay.hpp"
#include <algorithm>
#include <cstdio>
#include <FL/fl_ask.H>

/**
 * @brief Constructor for ServerSocket class, initializes Winsock and sets up the server socket.
 * @param _port The port number to bind the server socket.
 * @param _playerDisplay Pointer to the PlayerDisplay object for updating player list.
 */
ServerSocket::ServerSocket(int _port, PlayerDisplay* _playerDisplay)
    : m_socket(INVALID_SOCKET), playerDisplay(_playerDisplay)
{
    // Initialize Winsock
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

    // Bind the socket to the server address
    if (bind(m_socket, result->ai_addr, result->ai_addrlen) == SOCKET_ERROR) {
        freeaddrinfo(result);
        throw std::runtime_error("Failed to bind socket");
    }

    freeaddrinfo(result);

    // Listen for incoming client connections
    if (listen(m_socket, SOMAXCONN) == SOCKET_ERROR) {
        throw std::runtime_error("Failed to listen on socket");
    }

    u_long mode = 1; // Set non-blocking mode
    if (ioctlsocket(m_socket, FIONBIO, &mode) == SOCKET_ERROR) {
        throw std::runtime_error("Failed to set non-blocking mode");
    }
}

/**
 * @brief Destructor for ServerSocket class, cleans up the socket.
 */
ServerSocket::~ServerSocket()
{
    closeAllClients();
    closesocket(m_socket);
    WSACleanup();
}

/**
 * @brief Accepts a new client connection.
 * @return A shared pointer to the ClientSocket for the accepted client, or nullptr if no connection.
 */
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

/**
 * @brief Broadcasts a message to all connected clients.
 * @param message The message to send to all clients.
 */
void ServerSocket::broadcastMessage(const std::string& message)
{
    for (const auto& client : clients) {
        client->send(message);
    }
}

/**
 * @brief Closes all connected clients and removes them from the list.
 */
void ServerSocket::closeAllClients()
{
    for (auto& client : clients) {
        client->send("[SERVER]: Server is shutting down.");
        client->closed();
    }
    clients.clear();
}

/**
 * @brief Handles client connections and processes their messages.
 * Accepts new clients, manages username changes, and broadcasts messages to clients.
 */
 // Inside ServerSocket
void ServerSocket::handleClientConnections() {
    std::shared_ptr<ClientSocket> client = accept();
    if (client) {
        printf("Client Connected!\n");

        // Receive username upon connection
        std::string username;
        if (client->receive(username)) {
            client->setUsername(username);
            printf("Username received: %s\n", username.c_str());
            playerDisplay->addPlayer(username);

            // Announce new connection to all clients
            broadcastMessage("[SERVER]: " + username + " has joined the server.");
        }

        // Add the new client to the list
        clients.push_back(client);
    }

    // Temporary list to track disconnected clients
    std::vector<std::string> disconnectedUsernames;

    // Process messages from connected clients
    clients.erase(std::remove_if(clients.begin(), clients.end(),
        [&](const std::shared_ptr<ClientSocket>& c) {
            std::string message;
            std::string username = c->getUsername();
            if (c->receive(message)) {
                // Handle username change command
                if (message.rfind("/change_username ", 0) == 0) {
                    std::string newUsername = message.substr(17); // Extract the new username
                    bool usernameAvailable = true;
                    // Check if the username is already taken
                    for (const auto& existingClient : clients) {
                        if (existingClient->getUsername() == newUsername) {
                            usernameAvailable = false;
                            break;
                        }
                    }
                    // Respond to the client based on whether the username is available
                    if (usernameAvailable) {
                        // Remove the old username from player display
                        playerDisplay->removePlayer(c->getUsername());
                        // Update the client's username
                        c->setUsername(newUsername);
                        // Add the new username to the display
                        playerDisplay->addPlayer(newUsername);
                        // Broadcast the username change
                        broadcastMessage("[SERVER]: " + newUsername + " has changed their username.");
                    }
                    else {
                        // Show an FL_Alert to the user on failure
                        fl_alert("The username '%s' is already taken. Please choose another one.", newUsername.c_str());
                    }

                    return false;  // Don't remove the client yet, continue processing messages
                }
                else {
                    // Broadcast other messages
                    broadcastMessage(c->getUsername() + ": " + message);
                    return false;
                }
            }

            // Check if the client has closed (disconnected)
            if (c->closed()) {
                // Add the username to the list of disconnected users
                disconnectedUsernames.push_back(username);

                // Remove the client from the player display
                playerDisplay->removePlayer(username);

                // Return true to remove the client from the list
                return true;
            }

            return false;
        }), clients.end());

    // Now broadcast all disconnection messages
    for (const auto& username : disconnectedUsernames) {
        broadcastMessage("[SERVER]: " + username + " has disconnected.");
    }


}