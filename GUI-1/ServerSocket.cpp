#include "ServerSocket.h"
#include "PlayerDisplay.hpp"
#include <algorithm>
#include <cstdio>
#include <FL/fl_ask.H>

// Work with XML
#include "pugiconfig.hpp"
#include "pugixml.hpp"

/**
 * @brief Constructor for ServerSocket class, initializes Winsock and sets up the server socket.
 * @param _port The port number to bind the server socket.
 * @param _playerDisplay Pointer to the PlayerDisplay object for updating player list.
 */
ServerSocket::ServerSocket(int _port, PlayerDisplay* playerDisplay, const std::string& settingsPath)
    : m_socket(INVALID_SOCKET), playerDisplay(playerDisplay), m_settings(settingsPath)
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

    return std::make_shared<ClientSocket>(socket, playerDisplay, "config.xml");
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
 // Handle client connections and process messages, including whispers
void ServerSocket::handleClientConnections() {
    std::shared_ptr<ClientSocket> client = accept();

    if (client) {
        // Receive username upon connection
        std::string username;
        if (client->receive(username)) {
            client->setUsername(username);
            printf("Username received: %s\n", username.c_str());

            // Check that the username is not already in use
            if (isUsernameTaken(username)) {
                fl_alert("This Username is already in use!");
                return;
            }
            client->addingPlayer(username);
            // Announce new connection to all clients
            broadcastMessage("[SERVER]: " + username + " has joined the server.");
        }

        printf("Client Connected!\n");
        // Add the new client to the list
        clients.push_back(client);
    }

    // Temporary list to track disconnected clients
    std::vector<std::string> disconnectedUsernames;

    // Process messages from connected clients
    for (auto it = clients.begin(); it != clients.end(); /* no increment here */) {
        std::shared_ptr<ClientSocket>& c = *it;
        std::string message;
        std::string username = c->getUsername();

        // If the client has sent a message, handle it
        if (c->receive(message)) {
            // Handle whisper (direct message)
            // Handle whisper (direct message)
            if (message.rfind("W/", 0) == 0) {  // Check if the message starts with W/
                size_t spacePos = message.find(" ", 2);  // Find the first space after 'W/'
                if (spacePos != std::string::npos) {
                    std::string targetUsername = message.substr(2, spacePos - 2);  // Extract the target username
                    std::string whisperMessage = message.substr(spacePos + 1);  // Extract the whisper message (everything after the space)

                    // Find the target client by username
                    auto targetClient = std::find_if(clients.begin(), clients.end(),
                        [&](const std::shared_ptr<ClientSocket>& client) {
                            return client->getUsername() == targetUsername;
                        });

                    if (targetClient != clients.end()) {
                        // Send whisper message to the target user
                        (*targetClient)->send("[Whisper] " + c->getUsername() + ": " + whisperMessage);
                        // Optionally, send feedback to the sender
                        c->send("[Whisper] You to " + targetUsername + ": " + whisperMessage);
                    }
                    else {
                        // Target user not found, send error message to the sender
                        c->send("[SERVER]: User '" + targetUsername + "' not found.");
                    }
                }
                else {
                    // If there’s no space after the username, notify the client
                    c->send("[SERVER]: Invalid whisper format. Usage: W/username message");
                }
            }
            // Handle SV/ command to send server version privately
            else if (message.rfind("SV/", 0) == 0) {  // Check if the message starts with SV/
                std::string version = "Server Version: 1.0.0";  // Specify your version here
                c->send("[SERVER]: " + version);  // Send the version privately to the client
            }
            // Handle username change command
            else if (message.rfind("/change_username ", 0) == 0) {
                std::string newUsername = message.substr(17); // Extract the new username

                if (handleUsernameChange(c, newUsername)) {
                    // Broadcast the username change
                    broadcastMessage("[SERVER]: " + newUsername + " has changed their username.");
                }
                else {
                    // Notify the client that the username is already taken
                    c->send("[SERVER]: The username '" + newUsername + "' is already taken. Please choose another one.");
                }

                // Continue processing the same client
                ++it;
            }
            else {
                // Broadcast other messages
                broadcastMessage(c->getUsername() + ": " + message);
                ++it;
            }
        }
        else if (c->closed()) {
            // If the client has disconnected, process their disconnection
            disconnectedUsernames.push_back(username);
            c->removingPlayer(username);

            // Remove the client from the list
            it = clients.erase(it);
        }
        else {
            // No message received, continue to the next client
            ++it;
        }
    }

    // Now broadcast all disconnection messages
    for (const auto& username : disconnectedUsernames) {
        broadcastMessage("[SERVER]: " + username + " has disconnected.");
    }
}


/**
 * @brief Checks if the given username is already taken.
 * @param username The username to check.
 * @return true if the username is already in use, false otherwise.
 */
bool ServerSocket::isUsernameTaken(const std::string& username) {
    return std::any_of(clients.begin(), clients.end(),
        [&](const std::shared_ptr<ClientSocket>& client) {
            return client->getUsername() == username;
        });
}

/**
 * @brief Handles username change for a client.
 * @param client The client requesting the change.
 * @param newUsername The new username.
 * @return true if the username change was successful, false if the new username is already taken.
 */
 // Handles username change for a client.
bool ServerSocket::handleUsernameChange(std::shared_ptr<ClientSocket> client, const std::string& newUsername) {
    // Check if the new username is already taken
    if (isUsernameTaken(newUsername)) {
        return false;
    }

    // Proceed with the username change
    client->removingPlayer(client->getUsername());
    client->setUsername(newUsername);
    client->addingPlayer(newUsername);
    return true;
}
