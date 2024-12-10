#include <ws2tcpip.h>
#include "ClientSocket.h"
#include <stdexcept>
#include <vector>

// Constructor
ClientSocket::ClientSocket()
    : m_socket(INVALID_SOCKET), m_closed(false), m_username("") {}

// Constructor with existing socket
ClientSocket::ClientSocket(SOCKET socket)
    : m_socket(socket), m_closed(false), m_username("") {}

// Destructor
ClientSocket::~ClientSocket() {
    close();
}

// Connect to a server
bool ClientSocket::ConnectToServer(const std::string& address, int port) {
    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, address.c_str(), &serverAddr.sin_addr) <= 0) {
        return false;  // Invalid address
    }

    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket == INVALID_SOCKET) {
        throw std::runtime_error("Failed to create socket");
    }

    if (connect(m_socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(m_socket);
        return false;
    }

    return true;
}

// Receive a message
bool ClientSocket::receive(std::string& message) {
    char buffer[512];
    int bytesReceived = recv(m_socket, buffer, sizeof(buffer) - 1, 0);

    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        message = std::string(buffer);
        return true;
    }
    else if (bytesReceived == 0) {
        close(); // Connection closed by the server
    }

    return false;
}

// Send a message
void ClientSocket::send(const std::string& message) {
    if (!m_closed) {
        ::send(m_socket, message.c_str(), static_cast<int>(message.size()), 0);
    }
}

// Check if socket is closed
bool ClientSocket::closed() const {
    return m_closed;
}

// Close the socket
void ClientSocket::close() {
    if (!m_closed && m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
        m_closed = true;
    }
}

// Get username
std::string ClientSocket::getUsername() const {
    return m_username;
}

// Set username
void ClientSocket::setUsername(const std::string& username) {
    m_username = username;
}

bool ClientSocket::isConnected() const { 
    // Check if the socket is valid and not closed 
    return m_socket != INVALID_SOCKET && !m_closed; 
}
