#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "ServerSocket.h"
#include "ClientSocket.h"
#include <iostream>
#include <stdexcept>
#include <sstream>

ServerSocket::ServerSocket(int port) : serverSocket(INVALID_SOCKET) {
    configureSocket(port);
    retrieveHostIP();
    std::cout << "Server running on IP: " << serverIP << "\n";
}

ServerSocket::~ServerSocket() {
    for (const auto& client : clients) {
        client->sendMessage("shutdown", "Server is closing.");
    }
    closesocket(serverSocket);
    clients.clear();
}

void ServerSocket::configureSocket(int port) {
    addrinfo hints = {}, * result = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(nullptr, std::to_string(port).c_str(), &hints, &result) != 0) {
        throw std::runtime_error("Address resolution failed.");
    }

    serverSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (serverSocket == INVALID_SOCKET) {
        freeaddrinfo(result);
        throw std::runtime_error("Socket creation failed.");
    }

    if (bind(serverSocket, result->ai_addr, int(result->ai_addrlen)) == SOCKET_ERROR) {
        freeaddrinfo(result);
        throw std::runtime_error("Socket binding failed.");
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        throw std::runtime_error("Listening setup failed.");
    }

    u_long nonBlocking = 1;
    if (ioctlsocket(serverSocket, FIONBIO, &nonBlocking) == SOCKET_ERROR) {
        throw std::runtime_error("Failed to set non-blocking mode.");
    }

    freeaddrinfo(result);
}

void ServerSocket::retrieveHostIP() {
    sockaddr_in loopbackAddr = {};
    loopbackAddr.sin_family = AF_INET;
    loopbackAddr.sin_addr.s_addr = inet_addr("8.8.8.8"); // External address
    loopbackAddr.sin_port = htons(53); // Arbitrary port

    SOCKET tempSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (connect(tempSocket, reinterpret_cast<sockaddr*>(&loopbackAddr), sizeof(loopbackAddr)) == SOCKET_ERROR) {
        closesocket(tempSocket);
        throw std::runtime_error("Failed to connect for IP retrieval.");
    }

    sockaddr_in hostAddr;
    int addrLen = sizeof(hostAddr);
    if (getsockname(tempSocket, reinterpret_cast<sockaddr*>(&hostAddr), &addrLen) == SOCKET_ERROR) {
        closesocket(tempSocket);
        throw std::runtime_error("Failed to get host IP.");
    }

    char ipBuffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &hostAddr.sin_addr, ipBuffer, sizeof(ipBuffer));
    serverIP = ipBuffer;
    closesocket(tempSocket);
}

std::shared_ptr<ClientSocket> ServerSocket::acceptClient() {
    SOCKET clientSock = accept(serverSocket, nullptr, nullptr);
    if (clientSock == INVALID_SOCKET) {
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
            throw std::runtime_error("Client acceptance failed.");
        }
        return nullptr;
    }
    return std::make_shared<ClientSocket>(clientSock);
}

void ServerSocket::manageClients() {
    auto client = acceptClient();
    if (client) {
        clients.push_back(client);
        usernames.push_back("");
        std::cout << "New client connected. Total: " << clients.size() << "\n";
    }

    for (size_t i = 0; i < clients.size(); ++i) {
        std::string tag, message;
        if (clients[i]->receiveMessage(tag, message)) {
            if (tag == "username") {
                usernames[i] = message;
                for (const auto& otherClient : clients) {
                    otherClient->sendMessage("newUser", message);
                }
            }
            else if (tag == "message") {
                for (const auto& otherClient : clients) {
                    otherClient->sendMessage("broadcast", message);
                }
            }
        }

        if (clients[i]->isConnectionClosed()) {
            std::cout << "Client disconnected.\n";
            clients.erase(clients.begin() + i);
            usernames.erase(usernames.begin() + i);
            --i;
        }

    }
}

std::string ServerSocket::getServerIP() const {
    return serverIP;
}