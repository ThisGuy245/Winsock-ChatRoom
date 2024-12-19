#include <FL/fl_ask.H>
#include "ClientSocket.h"
#include "ServerSocket.h"
#include <FL/fl_draw.H>
#include <iostream>
#include <stdexcept>
#include <winsock2.h>
#include <ws2tcpip.h>

/**
 * @brief Constructor for ClientSocket when using an existing socket.
 * @param socket The socket to associate with this client.
 * @param playerDisplay Pointer to the PlayerDisplay instance.
 * @throws std::runtime_error if the socket is invalid.
 */
ClientSocket::ClientSocket(SOCKET socket, PlayerDisplay* playerDisplay)
    : m_socket(socket), m_closed(false), playerDisplay(playerDisplay) {
    if (socket == INVALID_SOCKET) {
        throw std::runtime_error("Invalid socket");
    }
}

/**
 * @brief Constructor for ClientSocket when creating a new connection to a server.
 * @param ipAddress The IP address of the server to connect to.
 * @param port The port number to connect to.
 * @param username The username to send to the server upon connection.
 * @param playerDisplay Pointer to the PlayerDisplay instance.
 * @throws std::runtime_error if the connection or socket creation fails.
 */
ClientSocket::ClientSocket(const std::string& ipAddress, int port, const std::string& username, PlayerDisplay* playerDisplay)
    : m_socket(INVALID_SOCKET), m_closed(false), m_username(username), playerDisplay(playerDisplay) {
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

    u_long mode = 1;
    if (ioctlsocket(m_socket, FIONBIO, &mode) == SOCKET_ERROR) {
        throw std::runtime_error("Failed to set non-blocking mode");
    }

    // Send username to the server upon connection
    send(m_username);
}

/**
 * @brief Destructor for ClientSocket class, cleans up the socket.
 */
ClientSocket::~ClientSocket() {
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
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

/**
 * @brief Sends a message to the server.
 * @param message The message to send.
 * @throws std::runtime_error if sending the message fails.
 */
void ClientSocket::send(const std::string& message) {
    int bytes = ::send(m_socket, message.c_str(), message.length(), 0);
    if (bytes <= 0) {
        throw std::runtime_error("Failed to send data");
    }
}

/**
 * @brief Receives a message from the server.
 * @param message The message received from the server.
 * @return True if the message was successfully received, otherwise false.
 */
bool ClientSocket::receive(std::string& message) {
    char buffer[512] = { 0 };
    int bytes = ::recv(m_socket, buffer, sizeof(buffer) - 1, 0);

    if (bytes == SOCKET_ERROR) {
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
            m_closed = true;  // Client is disconnected
            return false;
        }
        return false;
    }
    else if (bytes == 0) {  // Connection closed by the peer
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
    std::cout << "Adding player: " << username << std::endl;
    if (playerDisplay) {
        playerDisplay->addPlayer(username); // Add player to the display
    }
}

/**
 * @brief Removes a player from the player list and updates the PlayerDisplay.
 * @param username The username of the player to remove.
 */
void ClientSocket::removingPlayer(const std::string& username) {
    std::cout << "Removing player: " << username << std::endl;
    if (playerDisplay) {
        playerDisplay->removePlayer(username); // Remove player from the display
    }
}

/**
 * @brief Checks if the client socket is closed.
 * @return True if the socket is closed, otherwise false.
 */
bool ClientSocket::closed() {
    return m_closed;
}
