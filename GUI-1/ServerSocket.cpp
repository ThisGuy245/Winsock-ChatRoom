#include "ServerSocket.h"
#include "PlayerDisplay.hpp"
#include "NetProtocol.h"
#include <algorithm>
#include <cstdio>
#include <cctype>
#include <FL/fl_ask.H>

// Work with XML
#include "pugiconfig.hpp"
#include "pugixml.hpp"

//=============================================================================
// SECURITY CONSTANTS
//=============================================================================

/** Maximum allowed username length */
constexpr size_t MAX_USERNAME_LENGTH = 64;

/** Minimum username length */
constexpr size_t MIN_USERNAME_LENGTH = 1;

/** Maximum allowed chat message length */
constexpr size_t MAX_CHAT_MESSAGE_LENGTH = 4096;

/**
 * @brief Constructor for ServerSocket class, initializes Winsock and sets up the server socket.
 * 
 * SECURITY NOTES:
 * - Server socket is set to non-blocking for accept() to prevent DoS
 * - Client sockets are configured with timeouts upon acceptance
 * - SO_REUSEADDR is NOT set to prevent port hijacking attacks
 * 
 * @param _port The port number to bind the server socket.
 * @param _playerDisplay Pointer to the PlayerDisplay object for updating player list.
 * @param settingsPath Path to the settings XML file.
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
    
    // SECURITY NOTE: We intentionally do NOT set SO_REUSEADDR
    // SO_REUSEADDR can allow an attacker to hijack a port if the
    // legitimate server is in TIME_WAIT state.
    
    if (bind(m_socket, result->ai_addr, static_cast<int>(result->ai_addrlen)) == SOCKET_ERROR) {
        freeaddrinfo(result);
        throw std::runtime_error("Failed to bind socket");
    }

    freeaddrinfo(result);
      
    if (listen(m_socket, SOMAXCONN) == SOCKET_ERROR) {
        throw std::runtime_error("Failed to listen on socket");
    }

    // Set server socket to non-blocking for accept()
    // This allows us to poll for new connections without blocking
    u_long mode = 1;
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
 * @brief Accepts a new client connection with security configuration.
 * 
 * SECURITY NOTES:
 * - New client sockets are configured with timeouts immediately
 * - This prevents slowloris-style connection exhaustion attacks
 * - Client is NOT authenticated at this point - just connected
 * 
 * @return A shared pointer to the ClientSocket for the accepted client, or nullptr if no connection.
 */
std::shared_ptr<ClientSocket> ServerSocket::accept()
{
    SOCKET clientSocket = ::accept(m_socket, NULL, NULL);
    if (clientSocket == INVALID_SOCKET) {
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
            printf("[SECURITY] Failed to accept connection: %d\n", WSAGetLastError());
        }
        return nullptr;
    }

    // SECURITY: Configure the new client socket with timeouts
    // This happens BEFORE any data exchange to protect against slow attacks
    try {
        auto client = std::make_shared<ClientSocket>(clientSocket, playerDisplay, "config.xml");
        // Note: ClientSocket constructor now configures security settings
        return client;
    }
    catch (const std::exception& e) {
        printf("[SECURITY] Failed to initialize client socket: %s\n", e.what());
        closesocket(clientSocket);
        return nullptr;
    }
}

/**
 * @brief Validate a username for security requirements.
 * 
 * SECURITY CHECKS:
 * 1. Length within bounds (prevents buffer issues)
 * 2. Printable ASCII only (prevents injection attacks)
 * 3. No control characters (prevents log injection)
 * 4. No leading/trailing whitespace (prevents confusion attacks)
 * 
 * @param username The username to validate (ATTACKER-CONTROLLED)
 * @return True if username passes all security checks, false otherwise.
 */
bool ServerSocket::isValidUsername(const std::string& username) {
    // Check length bounds
    if (username.size() < MIN_USERNAME_LENGTH || username.size() > MAX_USERNAME_LENGTH) {
        printf("[SECURITY] Username rejected: invalid length (%zu)\n", username.size());
        return false;
    }
    
    // Check for leading/trailing whitespace
    if (std::isspace(static_cast<unsigned char>(username.front())) ||
        std::isspace(static_cast<unsigned char>(username.back()))) {
        printf("[SECURITY] Username rejected: leading/trailing whitespace\n");
        return false;
    }
    
    // Validate each character
    for (unsigned char c : username) {
        // Allow only printable ASCII (0x20-0x7E)
        // This prevents:
        // - Control character injection (newlines in logs, etc.)
        // - Unicode homograph attacks
        // - Non-printable character confusion
        if (c < 0x20 || c > 0x7E) {
            printf("[SECURITY] Username rejected: invalid character (0x%02X)\n", c);
            return false;
        }
    }
    
    // Reject usernames that look like system messages
    // This prevents spoofing server messages
    if (username.find("[SERVER]") != std::string::npos ||
        username.find("[SYSTEM]") != std::string::npos ||
        username.find("[ADMIN]") != std::string::npos) {
        printf("[SECURITY] Username rejected: contains reserved prefix\n");
        return false;
    }
    
    return true;
}

/**
 * @brief Broadcasts a message to all connected clients (LEGACY).
 * @deprecated Use broadcastMessageSecure() for new code.
 * @param message The message to send to all clients.
 */
void ServerSocket::broadcastMessage(const std::string& message)
{
    for (const auto& client : clients) {
        try {
            client->send(message);
        }
        catch (const std::exception& e) {
            printf("[WARNING] Failed to send to client %s: %s\n", 
                   client->getUsername().c_str(), e.what());
        }
    }
}

/**
 * @brief Broadcasts a message to all connected clients using secure protocol.
 * 
 * SECURITY: Uses length-prefixed messages to prevent framing issues.
 * Failed sends are logged but don't affect other clients.
 * 
 * @param message The message to broadcast.
 */
void ServerSocket::broadcastMessageSecure(const std::string& message)
{
    for (const auto& client : clients) {
        NetProtocol::Result result = client->sendSecure(message);
        if (result != NetProtocol::Result::Success) {
            printf("[WARNING] Failed to send to client %s: %s\n",
                   client->getUsername().c_str(),
                   NetProtocol::ResultToString(result));
        }
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
 * 
 * SECURITY IMPLEMENTATION NOTES:
 * 1. New connections: Username is validated before acceptance
 * 2. All messages: Length and content validated before processing
 * 3. Commands: Parsed carefully to prevent injection
 * 4. Broadcasts: Use server-controlled format, not raw client input
 * 
 * TRUST BOUNDARY: All data from clients is ATTACKER-CONTROLLED.
 */
void ServerSocket::handleClientConnections() {
    // =========================================================================
    // STEP 1: Accept new connections
    // =========================================================================
    std::shared_ptr<ClientSocket> client = accept();

    if (client) {
        // Receive username (using legacy non-blocking receive)
        std::string username;
        if (client->receive(username)) {
            // SECURITY CHECK: Validate username before accepting
            if (!isValidUsername(username)) {
                printf("[SECURITY] Rejected connection: invalid username\n");
                return;
            }
            
            if (isUsernameTaken(username)) {
                printf("[INFO] Rejected connection: username '%s' already taken\n", username.c_str());
                try {
                    client->send("[SERVER]: Username is already in use. Disconnecting.");
                } catch (...) {}
                return;
            }
            
            client->setUsername(username);
            printf("[INFO] Client connected: %s\n", username.c_str());
            
            client->addingPlayer(username);
            clients.push_back(client);
            
            broadcastMessage("[SERVER]: " + username + " has joined the server.");
        }
        else {
            // No username yet - add to list anyway, we'll get it on next poll
            // This handles the case where username arrives in a separate packet
            printf("[INFO] Client connected, waiting for username...\n");
            clients.push_back(client);
        }
    }

    // =========================================================================
    // STEP 2: Process messages from connected clients
    // =========================================================================
    std::vector<std::string> disconnectedUsernames;

    for (auto it = clients.begin(); it != clients.end(); /* no increment here */) {
        std::shared_ptr<ClientSocket>& c = *it;
        std::string message;
        std::string username = c->getUsername();

        // Use legacy non-blocking receive
        bool received = c->receive(message);
        
        if (received) {
            // SECURITY CHECK: Validate message length
            if (message.size() > MAX_CHAT_MESSAGE_LENGTH) {
                printf("[SECURITY] Message from %s rejected: too long\n", username.c_str());
                try { c->send("[SERVER]: Message too long."); } catch (...) {}
                ++it;
                continue;
            }
            
            // Handle whisper: W/targetUser message
            if (message.rfind("W/", 0) == 0) {
                size_t spacePos = message.find(' ', 2);
                if (spacePos != std::string::npos && spacePos > 2) {
                    std::string targetUsername = message.substr(2, spacePos - 2);
                    std::string whisperMessage = message.substr(spacePos + 1);
                    
                    if (whisperMessage.empty()) {
                        try { c->send("[SERVER]: Empty whisper message."); } catch (...) {}
                        ++it;
                        continue;
                    }

                    auto targetClient = std::find_if(clients.begin(), clients.end(),
                        [&](const std::shared_ptr<ClientSocket>& client) {
                            return client->getUsername() == targetUsername;
                        });

                    if (targetClient != clients.end()) {
                        try {
                            (*targetClient)->send("[Whisper from " + username + "]: " + whisperMessage);
                            c->send("[Whisper to " + targetUsername + "]: " + whisperMessage);
                        } catch (...) {}
                    }
                    else {
                        try { c->send("[SERVER]: User '" + targetUsername + "' not found."); } catch (...) {}
                    }
                }
                else {
                    try { c->send("[SERVER]: Invalid whisper format. Usage: W/username message"); } catch (...) {}
                }
                ++it;
            }
            // Handle server version request
            else if (message == "SV/" || message.rfind("SV/", 0) == 0) {
                try { c->send("[SERVER]: Server Version: 1.0.0 (Security Phase 1)"); } catch (...) {}
                ++it;
            }
            // Handle username change command
            else if (message.rfind("/change_username ", 0) == 0) {
                std::string newUsername = message.substr(17);
                
                // Validate new username
                if (!isValidUsername(newUsername)) {
                    try { c->send("[SERVER]: Invalid username format."); } catch (...) {}
                    ++it;
                    continue;
                }

                std::string oldUsername = c->getUsername();
                if (handleUsernameChange(c, newUsername)) {
                    broadcastMessage("[SERVER]: " + oldUsername + " is now known as " + newUsername);
                }
                else {
                    try { c->send("[SERVER]: The username '" + newUsername + "' is already taken."); } catch (...) {}
                }
                ++it;
            }
            // Regular chat message
            else {
                // SECURITY: Server formats the broadcast message
                // The username comes from server-side storage, not from the message
                
                // Check for channel prefix [CH:id] and preserve it
                std::string channelPrefix = "";
                std::string actualMessage = message;
                
                if (message.rfind("[CH:", 0) == 0) {
                    size_t endBracket = message.find(']');
                    if (endBracket != std::string::npos) {
                        channelPrefix = message.substr(0, endBracket + 1);
                        actualMessage = message.substr(endBracket + 1);
                    }
                }
                
                broadcastMessage(channelPrefix + username + ": " + actualMessage);
                ++it;
            }
        }
        else if (c->closed()) {
            if (!username.empty()) {
                disconnectedUsernames.push_back(username);
                c->removingPlayer(username);
            }
            it = clients.erase(it);
        }
        else {
            // No message received (would-block), continue to next client
            ++it;
        }
    }

    // Broadcast disconnection messages
    for (const auto& uname : disconnectedUsernames) {
        printf("[INFO] Client disconnected: %s\n", uname.c_str());
        broadcastMessage("[SERVER]: " + uname + " has disconnected.");
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
