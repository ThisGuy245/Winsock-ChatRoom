
#include "ServerSocket.h"
#include "ClientSocket.h"
#include <ws2tcpip.h> // For TCP/IP functionality
#include <string>
#include <stdexcept> // For throwing exceptions

// Constructor: Initializes the server socket and binds it to the specified port
ServerSocket::ServerSocket(int _port)
    : m_socket(INVALID_SOCKET) {
    addrinfo hints = { 0 };
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    addrinfo* result = NULL;

    if (getaddrinfo(NULL, std::to_string(_port).c_str(), &hints, &result) != 0) {
        throw std::runtime_error("Failed to resolve server address or port");
    }

    m_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (m_socket == INVALID_SOCKET) {
        freeaddrinfo(result);
        throw std::runtime_error("Failed to create socket!");
    }

    int opt = 1;
    if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
        throw std::runtime_error("setsockopt failed!");
    }

    if (bind(m_socket, result->ai_addr, result->ai_addrlen) == SOCKET_ERROR) {
        freeaddrinfo(result);
        closesocket(m_socket);
        throw std::runtime_error("Failed to bind socket!");
    }

    freeaddrinfo(result);

    if (listen(m_socket, SOMAXCONN) == SOCKET_ERROR) {
        throw std::runtime_error("Failed to listen on socket");
    }

    u_long mode = 1;
    if (ioctlsocket(m_socket, FIONBIO, &mode) == SOCKET_ERROR) {
        throw std::runtime_error("Failed to set non-blocking mode");
    }
}

// Destructor: Closes the server socket
ServerSocket::~ServerSocket() {
    closesocket(m_socket);
}

// Accept a new client connection
std::shared_ptr<ClientSocket> ServerSocket::accept() {
    SOCKET clientSocket = ::accept(m_socket, NULL, NULL);
    if (clientSocket == INVALID_SOCKET) {
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
            throw std::runtime_error("Failed to accept socket");
        }
        return nullptr;
    }

    auto newClient = std::make_shared<ClientSocket>(clientSocket);
    m_clients.push_back(newClient);

    return newClient;
}

// Get the list of connected clients
std::vector<std::shared_ptr<ClientSocket>>& ServerSocket::getClients() {
    return m_clients;
}

// Add a new client to the list
void ServerSocket::addClient(std::shared_ptr<ClientSocket> client) {
    m_clients.push_back(client);
}

// Remove a client from the list
void ServerSocket::removeClient(std::shared_ptr<ClientSocket> client) {
    m_clients.erase(std::remove(m_clients.begin(), m_clients.end(), client), m_clients.end());
}

// Broadcast a message to all clients
void ServerSocket::broadcastMessage(const std::string& message) {
    for (auto& client : m_clients) {
        client->send(message);
    }
}

// Broadcast the list of usernames
void ServerSocket::broadcastUserList() {
    std::string userList = "Users in the lobby:\n";
    for (auto& client : m_clients) {
        userList += client->getUsername() + "\n";
    }
    broadcastMessage(userList);
}