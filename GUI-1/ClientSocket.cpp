#include "ClientSocket.h"
#include "ServerSocket.h"
#include "PlayerDisplay.hpp"
#include <FL\fl_ask.H>
#include <FL/fl_draw.H>

// Constructor for existing sockets
ClientSocket::ClientSocket(SOCKET socket) : m_socket(socket), m_closed(false) {
    if (socket == INVALID_SOCKET) {
        throw std::runtime_error("Invalid socket");
    }
}

PlayerDisplay* globalPlayerDisplay = nullptr;


// Constructor for new client connections
ClientSocket::ClientSocket(const std::string& ipAddress, int port, const std::string& username)
    : m_socket(INVALID_SOCKET), m_closed(false), m_username(username) {
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

    if (globalPlayerDisplay) {
        globalPlayerDisplay->addPlayer(m_username);
    }
}

ClientSocket::~ClientSocket() {
    if (globalPlayerDisplay) {
        globalPlayerDisplay->removePlayer(m_username);
    }
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
    }
}

void ClientSocket::setUsername(const std::string& username) {
    if (globalPlayerDisplay) {
        globalPlayerDisplay->removePlayer(m_username);
        globalPlayerDisplay->addPlayer(username);
    }
    m_username = username;
}

const std::string& ClientSocket::getUsername() const {
    return m_username;
}

void ClientSocket::send(const std::string& message) {
    int bytes = ::send(m_socket, message.c_str(), message.length(), 0);
    if (bytes <= 0) {
        throw std::runtime_error("Failed to send data");
    }
}

// ClientSocket::receive()
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

void ClientSocket::changeUsername(const std::string& newUsername) {
    std::string command = "/change_username " + newUsername;
    send(command);  // Send the command to the server

    // Update the local username immediately
    setUsername(newUsername);

    // Optionally, you can listen for a confirmation from the server to update the UI
    std::string response;
    if (receive(response)) {
        if (response == "USERNAME_CHANGED") {
            // If server confirms the change, update the display
            if (globalPlayerDisplay) {
                globalPlayerDisplay->removePlayer(m_username);  // Remove old username
                globalPlayerDisplay->addPlayer(newUsername);    // Add new username
            }
            fl_alert("Your username has been successfully changed to '%s'.", newUsername.c_str());
        }
        else if (response == "USERNAME_TAKEN") {
            // Handle failure case (username already taken, invalid, etc.)
            fl_alert("Failed to change username. The username '%s' is already taken. Please try again.", newUsername.c_str());
        }
    }
}




bool ClientSocket::closed() {
    return m_closed;
}
