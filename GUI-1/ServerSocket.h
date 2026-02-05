#ifndef SERVERSOCKET_H
#define SERVERSOCKET_H

/**
 * @file ServerSocket.h
 * @brief Server-side socket management with secure message handling
 * 
 * SECURITY NOTES:
 * - All client data is UNTRUSTED by default
 * - Message framing uses NetProtocol for length-prefixed messages
 * - Client connections have timeouts to prevent resource exhaustion
 * 
 * TRUST BOUNDARY:
 * Everything received from clients is ATTACKER-CONTROLLED.
 * The server must validate all input before processing.
 */

#include <winsock2.h>
#include <memory>
#include <vector>
#include <string>
#include "ClientSocket.h"
#include "PlayerDisplay.hpp"
#include "Settings.h"
#include "NetProtocol.h"

struct ServerSocket
{
public:
    /**
     * @brief List of connected clients
     * 
     * SECURITY NOTE: Each client in this list has passed initial connection
     * but is NOT authenticated. Treat all client data as hostile.
     */
    std::vector<std::shared_ptr<ClientSocket>> clients;
    
    /**
     * @brief Constructor that initializes the server socket.
     * @param _port The port number on which the server will listen.
     * @param _playerDisplay A pointer to the PlayerDisplay instance.
     * @param settingsPath Path to the settings XML file.
     */
    ServerSocket(int _port, PlayerDisplay* playerDisplay, const std::string& settingsPath);

    /** Destructor that cleans up server resources. */
    ~ServerSocket();

    /**
     * @brief Accepts a new client connection.
     * 
     * SECURITY: New connections are configured with timeouts
     * to prevent slowloris-style resource exhaustion attacks.
     * 
     * @return A shared pointer to the accepted ClientSocket, or nullptr if no connection.
     */
    std::shared_ptr<ClientSocket> accept();

    /** Closes all client connections. */
    void closeAllClients();

    /**
     * @brief Broadcasts a message to all connected clients using secure protocol.
     * @param message The message to broadcast.
     */
    void broadcastMessage(const std::string& message);
    
    /**
     * @brief Broadcasts a message using the secure protocol.
     * @param message The message to broadcast.
     */
    void broadcastMessageSecure(const std::string& message);

    /**
     * @brief Check if a username is already in use.
     * 
     * SECURITY NOTE: This is a basic uniqueness check, not authentication.
     * An attacker can still claim to be any username they want.
     * Real authentication requires Phase 2 (passwords/credentials).
     * 
     * @param username The username to check.
     * @return True if username is taken, false otherwise.
     */
    bool isUsernameTaken(const std::string& username);
    
    /**
     * @brief Handle a username change request.
     * @param client The client requesting the change.
     * @param newUsername The requested new username.
     * @return True if change was successful, false if username is taken.
     */
    bool handleUsernameChange(std::shared_ptr<ClientSocket> client, const std::string& newUsername);

    /** 
     * @brief Handles all client connections and incoming messages.
     * 
     * SECURITY: This is the main message processing loop.
     * All input validation should happen here before acting on messages.
     */
    void handleClientConnections();

    PlayerDisplay* playerDisplay;
    Settings m_settings;

private:
    SOCKET m_socket;
    
    /**
     * @brief Validate a username for security requirements.
     * 
     * @param username The username to validate.
     * @return True if username is valid, false otherwise.
     */
    bool isValidUsername(const std::string& username);
    
    // Disable copying and assignment
    ServerSocket(const ServerSocket& _copy) = delete;
    ServerSocket& operator=(const ServerSocket& _assign) = delete; 
};

#endif // SERVERSOCKET_H
