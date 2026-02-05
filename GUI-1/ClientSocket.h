#ifndef CLIENTSOCKET_H
#define CLIENTSOCKET_H

/**
 * @file ClientSocket.h
 * @brief Client-side socket wrapper with secure message handling
 * 
 * SECURITY NOTES:
 * - All network I/O now uses NetProtocol for length-prefixed framing
 * - Maximum message sizes are enforced
 * - Partial reads/writes are handled correctly
 * 
 * TRUST BOUNDARY:
 * Data received via receive() is ATTACKER-CONTROLLED.
 * Always validate before use.
 */

#include <string>
#include <stdexcept>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "PlayerDisplay.hpp"
#include "Settings.h"
#include "NetProtocol.h"
#include <tuple>

class MainWindow;

class ClientSocket {
public:
    // Constructors
    explicit ClientSocket(SOCKET socket, PlayerDisplay* playerDisplay, const std::string& settings);
    ClientSocket(const std::string& ipAddress, int port, const std::string& username,
        PlayerDisplay* playerDisplay, const std::string& settings, MainWindow* mainWindow);

    // Destructor
    ~ClientSocket();

    // =========================================================================
    // SECURE MESSAGE I/O
    // These methods use length-prefixed framing via NetProtocol
    // =========================================================================
    
    /**
     * @brief Send a message using secure length-prefixed protocol
     * @param message The message to send (max 64KB)
     * @return NetProtocol::Result indicating success or failure type
     */
    NetProtocol::Result sendSecure(const std::string& message);
    
    /**
     * @brief Receive a message using secure length-prefixed protocol
     * @param message Output: the received message
     * @return NetProtocol::Result indicating success or failure type
     * 
     * WARNING: The returned message is ATTACKER-CONTROLLED.
     * Validate all content before use.
     */
    NetProtocol::Result receiveSecure(std::string& message);

    // =========================================================================
    // LEGACY METHODS (deprecated - use sendSecure/receiveSecure)
    // Kept for backward compatibility during transition
    // =========================================================================
    
    void setUsername(const std::string& username);
    const std::string& getUsername() const;
    
    /** @deprecated Use sendSecure() instead */
    void send(const std::string& message);
    
    /** @deprecated Use receiveSecure() instead */
    bool receive(std::string& message);
    
    /** @deprecated Use receiveSecure() and parse result instead */
    bool receive(std::string& sender, std::string& message);
    
    void changeUsername(const std::string& newUsername);
    void addingPlayer(const std::string& username);
    void removingPlayer(const std::string& username);
    void updateLocalSettings(const std::string& settingsData);
    void applyUserSettings();
    void updateResolution(int width, int height);
    void toggleDarkMode();
    bool closed() const;
    
    /**
     * @brief Get the underlying socket (use with caution)
     * @return The raw SOCKET handle
     */
    SOCKET getSocket() const { return m_socket; }

    Settings m_settings;
    PlayerDisplay* playerDisplay;

private:
    friend struct ServerSocket;
    SOCKET m_socket;
    bool m_closed;
    std::string m_username;
    MainWindow* mainWindow;
};

#endif // CLIENTSOCKET_H

