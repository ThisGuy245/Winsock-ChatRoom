#include "ServerSocket.h"
#include "ClientSocket.h"
#include <ws2tcpip.h> // For TCP/IP functionality
#include <string>
#include <stdexcept> // For throwing exceptions

template class std::vector<std::shared_ptr<ClientSocket>>;

// Constructor: Initializes the server socket and binds it to the specified port
ServerSocket::ServerSocket(int _port)
    : m_socket(INVALID_SOCKET)
{
    // Fill in the server information (listen mode, using TCP and IPv4)
    addrinfo hints = { 0 };
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    addrinfo* result = NULL; // Address resolution for all addresses

    // Resolve the server address and port
    if (getaddrinfo(NULL, std::to_string(_port).c_str(), &hints, &result) != 0) {
        throw std::runtime_error("Failed to resolve server address or port");
    }

    // Create the server socket
    m_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (m_socket == INVALID_SOCKET) {
        freeaddrinfo(result);
        throw std::runtime_error("Failed to create socket!");
    }

    // Allow address reuse
    int opt = 1;
    if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
        throw std::runtime_error("setsockopt failed!");
    }

    // Bind the server socket to the address and port
    if (bind(m_socket, result->ai_addr, result->ai_addrlen) == SOCKET_ERROR) {
        freeaddrinfo(result);
        closesocket(m_socket);
        throw std::runtime_error("Failed to bind socket!");
    }

    freeaddrinfo(result); // Clean up address info


    // Start listening for incoming connections
    if (listen(m_socket, SOMAXCONN) == SOCKET_ERROR) {
        throw std::runtime_error("Failed to listen on socket");
    }

    // Set the server socket to non-blocking mode
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
int nextPlayerId = 1;  // Start player ID from 1

std::shared_ptr<ClientSocket> ServerSocket::accept() {
    SOCKET clientSocket = ::accept(m_socket, NULL, NULL);
    if (clientSocket == INVALID_SOCKET) {
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
            throw std::runtime_error("Failed to accept socket");
        }
        return nullptr;
    }

    auto newClient = std::make_shared<ClientSocket>(clientSocket);
    newClient->setPlayerId(nextPlayerId++);  // Assign a unique ID to each player
    m_clients.push_back(newClient);
    std::string message = "Player " + std::to_string(newClient->getPlayerId()) + " joined the lobby.";
    broadcastMessage(message);
    return newClient; // Return new client with a unique ID
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

void ServerSocket::broadcastMessage(const std::string& message) {
    for (auto& client : m_clients) {
        client->send(message);
    }
}

// In the server socket, broadcast the new player to all connected clients
void ServerSocket::broadcastPlayerList() {
    std::string playerList = "Players in the lobby:\n";
    for (auto& client : m_clients) {
        playerList += "Player " + std::to_string(client->getPlayerId()) + "\n";
    }
    for (auto& client : m_clients) {
        client->send(playerList); // Send updated player list to each client
    }
}
