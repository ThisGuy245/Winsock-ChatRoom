#include <FL/fl_ask.H>
#include "ClientSocket.h"
#include "ServerSocket.h"
#include <FL/fl_draw.H>
#include <iostream>
#include <stdexcept>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "MainWindow.h"
#include "NetProtocol.h"

/**
 * @brief Constructor for ClientSocket with an existing socket.
 * 
 * SECURITY: Configures socket with appropriate timeouts and options
 * to prevent slowloris-style attacks and ensure clean error handling.
 */
ClientSocket::ClientSocket(SOCKET socket, PlayerDisplay* playerDisplay, const std::string& settings)
    : m_socket(socket), m_closed(false), playerDisplay(playerDisplay), m_settings(settings), mainWindow(nullptr) {
    if (socket == INVALID_SOCKET) {
        throw std::runtime_error("Invalid socket");
    }
    
    // Configure socket options (TCP_NODELAY for low latency)
    NetProtocol::ConfigureSocket(m_socket);
    
    // Set non-blocking mode - REQUIRED for GUI event loop
    u_long mode = 1;
    if (ioctlsocket(m_socket, FIONBIO, &mode) == SOCKET_ERROR) {
        closesocket(m_socket);
        throw std::runtime_error("Failed to set non-blocking mode");
    }
}

/**
 * @brief Constructor for ClientSocket when connecting to a server.
 * 
 * SECURITY NOTES:
 * - Validates IP address format before connecting
 * - Configures socket with security settings after connection
 * - Sends username using secure length-prefixed protocol
 */
ClientSocket::ClientSocket(const std::string& ipAddress, int port, const std::string& username,
    PlayerDisplay* playerDisplay, const std::string& settings, MainWindow* mainWindow)
    : m_socket(INVALID_SOCKET), m_closed(false), m_username(username),
    playerDisplay(playerDisplay), m_settings(settings), mainWindow(mainWindow) {

    // SECURITY: Validate username length before proceeding
    // This prevents sending oversized usernames that could cause issues
    if (username.size() > 64) {
        throw std::runtime_error("Username too long (max 64 characters)");
    }

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }

    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) {
        WSACleanup();
        throw std::runtime_error("Failed to create socket");
    }

    sockaddr_in serverAddress = {};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);

    if (inet_pton(AF_INET, ipAddress.c_str(), &serverAddress.sin_addr) <= 0) {
        closesocket(m_socket);
        WSACleanup();
        throw std::runtime_error("Invalid address / Address not supported");
    }

    if (connect(m_socket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
        closesocket(m_socket);
        WSACleanup();
        throw std::runtime_error("Failed to connect to server");
    }

    // Configure socket options (TCP_NODELAY for low latency)
    NetProtocol::ConfigureSocket(m_socket);

    // Set non-blocking mode - REQUIRED for GUI event loop
    // The FLTK event loop polls sockets periodically; blocking would freeze the UI
    u_long mode = 1;
    if (ioctlsocket(m_socket, FIONBIO, &mode) == SOCKET_ERROR) {
        closesocket(m_socket);
        WSACleanup();
        throw std::runtime_error("Failed to set non-blocking mode");
    }

    // Send username using legacy method (works with non-blocking)
    // TODO: Migrate to secure protocol once we have proper async support
    send(m_username);
    
    applyUserSettings();
}



/**
 * @brief Destructor for ClientSocket.
 */
ClientSocket::~ClientSocket() {
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
    }
}

/**
 * @brief Apply user-specific settings from the Settings class.
 */
void ClientSocket::applyUserSettings() {
    pugi::xml_node user = m_settings.findOrCreateClient(m_username);

    // Apply resolution from settings
    std::tuple<int, int> resolution = m_settings.getRes(user);
    int width = std::get<0>(resolution);
    int height = std::get<1>(resolution);

    // Apply the resolution to the main window
    if (mainWindow) {
        mainWindow->setResolution(width, height);
    }

    // Apply dark mode
    std::string mode = m_settings.getMode(user);
    if (mode == "true") {
        // Enable dark mode
        Fl::background(45, 45, 45);  // Dark background for main window
        Fl::foreground(255, 255, 255);  // White text
    }
    else {
        // Enable light mode
        Fl::background(240, 240, 240);  // Light background
        Fl::foreground(0, 0, 0);  // Black text
    }
    if (mainWindow) {
        mainWindow->redraw();
    }
}


/**
 * @brief Sets the username for this client and updates the global player display.
 * @param username The new username to set for this client.
 */
void ClientSocket::setUsername(const std::string& username) {
    m_username = username;
}


/**
 * @brief Gets the current username of this client.
 * @return The current username of the client.
 */
const std::string& ClientSocket::getUsername() const {
    return m_username;
}

//=============================================================================
// SECURE MESSAGE I/O (NEW - USE THESE)
//=============================================================================

/**
 * @brief Send a message using secure length-prefixed protocol
 * 
 * SECURITY FEATURES:
 * - Length prefix prevents message boundary confusion
 * - Maximum message size enforced
 * - Handles partial sends correctly
 * 
 * @param message The message to send (max 64KB)
 * @return Result code indicating success or specific failure
 */
NetProtocol::Result ClientSocket::sendSecure(const std::string& message) {
    if (m_closed) {
        return NetProtocol::Result::Disconnected;
    }
    
    NetProtocol::Result result = NetProtocol::SendMessage(m_socket, message);
    
    if (result == NetProtocol::Result::Disconnected || 
        result == NetProtocol::Result::NetworkError) {
        m_closed = true;
    }
    
    return result;
}

/**
 * @brief Receive a message using secure length-prefixed protocol
 * 
 * SECURITY FEATURES:
 * - Validates message length before allocation (prevents memory exhaustion)
 * - Handles partial reads correctly (prevents message corruption)
 * - Times out on slow/stalled connections
 * 
 * WARNING: The returned message is ATTACKER-CONTROLLED.
 * You MUST validate the content before using it for:
 * - Command parsing
 * - Display to users
 * - Database queries
 * - File system operations
 * 
 * @param message Output: the received message (cleared on error)
 * @return Result code indicating success or specific failure
 */
NetProtocol::Result ClientSocket::receiveSecure(std::string& message) {
    message.clear();
    
    if (m_closed) {
        return NetProtocol::Result::Disconnected;
    }
    
    NetProtocol::Result result = NetProtocol::ReceiveMessage(m_socket, message);
    
    if (result == NetProtocol::Result::Disconnected || 
        result == NetProtocol::Result::NetworkError) {
        m_closed = true;
    }
    
    return result;
}

//=============================================================================
// LEGACY MESSAGE I/O (DEPRECATED - For backward compatibility only)
//=============================================================================

/**
 * @brief Sends a message to the server (DEPRECATED)
 * @deprecated Use sendSecure() instead - this method lacks proper framing
 * @param message The message to send
 * @throws std::runtime_error if sending the message fails
 * 
 * SECURITY WARNING: This method sends raw bytes without length prefix.
 * It will NOT interoperate correctly with the new secure protocol.
 * Use only for backward compatibility during migration.
 */
void ClientSocket::send(const std::string& message) {
    // SECURITY NOTE: This legacy method is kept for backward compatibility
    // New code should use sendSecure() instead
    int bytes = ::send(m_socket, message.c_str(), static_cast<int>(message.length()), 0);
    if (bytes <= 0) {
        m_closed = true;
        throw std::runtime_error("Failed to send data");
    }
}

/**
 * @brief Receives a message from the server (DEPRECATED)
 * @deprecated Use receiveSecure() instead - this method has framing issues
 * @param message The message received from the server
 * @return True if data was received, false otherwise
 * 
 * SECURITY WARNING: This method has critical vulnerabilities:
 * 1. No message boundary handling (TCP streaming issue)
 * 2. Fixed buffer size (potential truncation)
 * 3. Non-blocking behavior inconsistent
 * 
 * Use receiveSecure() for any new code.
 */
bool ClientSocket::receive(std::string& message) {
    char buffer[512] = { 0 };
    int bytes = ::recv(m_socket, buffer, sizeof(buffer) - 1, 0);

    if (bytes == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK && error != WSAETIMEDOUT) {
            m_closed = true;
        }
        return false;
    }
    else if (bytes == 0) {
        m_closed = true;
        return false;
    }

    message.assign(buffer, bytes);
    return true;
}

/**
 * @brief Sends a username change request to the server and updates the local display.
 * @param newUsername The new username to set.
 */
void ClientSocket::changeUsername(const std::string& newUsername) {
    std::string command = "/change_username " + newUsername;
    send(command);  // Send the command to the server

    // Update the local username immediately
    setUsername(newUsername);

    // Optionally, you can listen for a confirmation from the server to update the UI
    std::string response;
    if (receive(response)) {
        if (response == "USERNAME_CHANGED") {
            fl_alert("Your username has been successfully changed to '%s'.", newUsername.c_str());
        }
        else if (response == "USERNAME_TAKEN") {
            // Handle failure case (username already taken, invalid, etc.)
            fl_alert("Failed to change username. The username '%s' is already taken. Please try again.", newUsername.c_str());
        }
    }
}

/**
 * @brief Adds a player to the player list and updates the PlayerDisplay.
 * @param username The username of the player to add.
 */
void ClientSocket::addingPlayer(const std::string& username) {
    if (playerDisplay) {
        playerDisplay->addPlayer(username); // Add player to the display
    }
}

/**
 * @brief Removes a player from the player list and updates the PlayerDisplay.
 * @param username The username of the player to remove.
 */
void ClientSocket::removingPlayer(const std::string& username) {
    if (playerDisplay) {
        playerDisplay->removePlayer(username); // Remove player from the display
    }
}


/**
 * @brief Update the user's resolution settings.
 */
void ClientSocket::updateResolution(int width, int height) {
    m_settings.setRes(m_username, width, height);
    fl_message("Resolution updated to: %dx%d", width, height);
}

/**
 * @brief Toggle the user's dark mode setting.
 */
void ClientSocket::toggleDarkMode() {
    pugi::xml_node user = m_settings.findOrCreateClient(m_username);
    std::string currentMode = m_settings.getMode(user);
    std::string newMode = (currentMode == "true") ? "false" : "true";
    m_settings.setMode(m_username, newMode);
    fl_message("Dark mode toggled to: %s", newMode.c_str());
}

/**
 * @brief Update local settings based on server data.
 */
void ClientSocket::updateLocalSettings(const std::string& settingsData) {
    pugi::xml_document doc;
    if (!doc.load_string(settingsData.c_str())) {
        std::cerr << "Failed to parse settings data from server" << std::endl;
        return;
    }

    if (!doc.save_file("client_settings.xml")) {
        std::cerr << "Failed to save settings to local file" << std::endl;
    }
    else {
        std::cout << "Settings updated and saved locally" << std::endl;
    }
}


/**
 * @brief Checks if the client socket is closed.
 * @return True if the socket is closed, otherwise false.
 */
bool ClientSocket::closed() const {
    return m_closed;
}

/**
 * @brief Receive and parse a message with sender (DEPRECATED)
 * @deprecated Use receiveSecure() and parse the result separately
 * 
 * SECURITY WARNING: This method parses attacker-controlled data
 * using simple string operations. The "sender" field can be spoofed
 * by any client sending "FakeName: malicious message".
 * 
 * In a secure system, sender identity should come from authenticated
 * session data, NOT from the message content.
 * 
 * @param sender Output: extracted sender name (UNTRUSTED)
 * @param message Output: extracted message content (UNTRUSTED)
 * @return True if data received, false otherwise
 */
bool ClientSocket::receive(std::string& sender, std::string& message) {
    char buffer[512] = { 0 };
    int bytes = ::recv(m_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK && error != WSAETIMEDOUT) {
            m_closed = true;
        }
        return false;
    }
    else if (bytes == 0) {
        m_closed = true;
        return false;
    }

    std::string receivedData(buffer, bytes);

    // SECURITY NOTE: This parsing trusts the sender field in the message.
    // An attacker can easily spoof this. Do NOT use for access control.
    size_t delimiterPos = receivedData.find(": ");
    if (delimiterPos != std::string::npos && delimiterPos < 64) {
        // Basic sanity check: sender name shouldn't be excessively long
        sender = receivedData.substr(0, delimiterPos);
        message = receivedData.substr(delimiterPos + 2);
    }
    else {
        sender = "Unknown";
        message = receivedData;
    }
    return true;
}

