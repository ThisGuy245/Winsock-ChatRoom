#include "ClientSocket.h"
#include <stdexcept>
#include <iostream>
#include <sstream>

ClientSocket::ClientSocket(SOCKET sock) : clientSocket(sock), closed(false) {
    if (clientSocket == INVALID_SOCKET) {
        throw std::runtime_error("Invalid socket.");
    }
}

ClientSocket::ClientSocket(const std::string& serverIP, int serverPort) : clientSocket(INVALID_SOCKET), closed(false) {
    configureSocket(serverIP, serverPort);
}

ClientSocket::~ClientSocket() {
    closesocket(clientSocket);
}

void ClientSocket::configureSocket(const std::string& serverIP, int serverPort) {
    clientSocket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        throw std::runtime_error("Socket creation failed.");
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr);

    if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(clientSocket);
        throw std::runtime_error("Failed to connect to server.");
    }

    u_long nonBlocking = 1;
    if (ioctlsocket(clientSocket, FIONBIO, &nonBlocking) == SOCKET_ERROR) {
        throw std::runtime_error("Failed to set non-blocking mode.");
    }
}

void ClientSocket::sendMessage(const std::string& tag, const std::string& message) {
    if (this->closed) {
        throw std::runtime_error("Cannot send message; connection closed.");
    }

    std::ostringstream formattedMessage;
    formattedMessage << tag << ":" << message << "\n";

    std::string toSend = formattedMessage.str();
    int result = send(clientSocket, toSend.c_str(), static_cast<int>(toSend.size()), 0);
    if (result == SOCKET_ERROR) {
        throw std::runtime_error("Failed to send message.");
    }
}

bool ClientSocket::receiveMessage(std::string& tag, std::string& message) {
    if (this->closed) return false;

    char buffer[512];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived == 0) {
        this->closed = true;
        return false;
    }
    if (bytesReceived == SOCKET_ERROR) {
        if (WSAGetLastError() == WSAEWOULDBLOCK) return false;
        throw std::runtime_error("Failed to receive message.");
    }

    buffer[bytesReceived] = '\0';
    std::string receivedMessage(buffer);

    size_t delimiter = receivedMessage.find(':');
    if (delimiter != std::string::npos) {
        tag = receivedMessage.substr(0, delimiter);
        message = receivedMessage.substr(delimiter + 1);
        return true;
    }

    return false;
}

bool ClientSocket::isConnectionClosed() const {
    return this->closed;
}
