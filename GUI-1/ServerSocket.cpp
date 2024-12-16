#include <WS2tcpip.h>
#include <stdexcept>
#include <string>
#include <iostream>
#include "ServerSocket.h"

ServerSocket::ServerSocket(int _port) : m_socket(INVALID_SOCKET) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }

    addrinfo hints = { 0 };
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    addrinfo* result = nullptr;

    // Resolve server address and port
    if (getaddrinfo(NULL, std::to_string(_port).c_str(), &hints, &result) != 0) {
        throw std::runtime_error("Failed to resolve server address or port");
    }

    // Create the server socket
    m_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (m_socket == INVALID_SOCKET) {
        freeaddrinfo(result);
        throw std::runtime_error("Failed to create socket");
    }

    // Bind the server socket
    if (bind(m_socket, result->ai_addr, result->ai_addrlen) == SOCKET_ERROR) {
        freeaddrinfo(result);
        throw std::runtime_error("Failed to bind socket");
    }

    freeaddrinfo(result);

    // Set socket to listen mode
    if (listen(m_socket, SOMAXCONN) == SOCKET_ERROR) {
        throw std::runtime_error("Failed to listen on socket");
    }

    // Set the socket to non-blocking mode
    u_long mode = 1;
    if (ioctlsocket(m_socket, FIONBIO, &mode) == SOCKET_ERROR) {
        throw std::runtime_error("Failed to set non-blocking");
    }
}

ServerSocket::~ServerSocket() {
    closesocket(m_socket);
    WSACleanup();
}

std::shared_ptr<ClientSocket> ServerSocket::accept() {
    SOCKET clientSocket = ::accept(m_socket, NULL, NULL);
    if (clientSocket == INVALID_SOCKET) {
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
            throw std::runtime_error("Failed to accept socket");
        }
        return nullptr;
    }

    // Return a new ClientSocket instance
    return std::make_shared<ClientSocket>(clientSocket);
}

void ServerSocket::handleClientConnections() {
    // Accept new client connections
    std::shared_ptr<ClientSocket> client = accept();
    if (client) {
        std::string username;
        std::string welcomeMessage;

        // Receive username when the client connects
        if (client->receive(username, welcomeMessage)) {
            clientUsernames[client] = username;
            printf("%s Connected!\n", username.c_str());
            clients.push_back(client);
            broadcastMessage(username + " has joined the chat.");
        }
    }

    // Handle messages from connected clients
    for (int i = 0; i < clients.size(); ++i) {
        std::string sender, message;
        bool receivedMessage = clients[i]->receive(sender, message);

        if (receivedMessage) {
            std::string fullMessage = sender + ": " + message;
            printf("Message received: %s\n", fullMessage.c_str());
            broadcastMessage(fullMessage, clients[i]); // Exclude sender
        }
        else if (clients[i]->closed()) {
            std::string disconnectedUser = clientUsernames[clients[i]];
            printf("%s Disconnected\n", disconnectedUser.c_str());

            // Notify others of disconnection
            broadcastMessage(disconnectedUser + " has left the chat.");

            // Remove the client
            clients.erase(clients.begin() + i);
            clientUsernames.erase(clients[i]);
            --i; // Adjust index for removed client
        }
    }
}

void ServerSocket::broadcastMessage(const std::string& message, std::shared_ptr<ClientSocket> excludeClient) {
    for (auto& client : clients) {
        if (client != excludeClient) {
            client->send("Server", message); // Send with "Server" as the sender for broadcast
        }
    }
}
