#include "ClientSocket.h"
#include "ServerSocket.h"

// Constructor for already-existing sockets
ClientSocket::ClientSocket(SOCKET socket) : m_socket(socket), m_closed(false) {
    if (socket == INVALID_SOCKET) {
        throw std::runtime_error("Invalid socket");
    }
}

// Constructor for new ClientSockets
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

    sockaddr_in serverAddress;
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
        throw std::runtime_error("Failed to set non-blocking");
    }

    printf("Connected to server at %s:%d\n", ipAddress.c_str(), port);
}

ClientSocket::~ClientSocket() {
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
    }
}

void ClientSocket::setUsername(const std::string& username) {
    m_username = username;
}

const std::string& ClientSocket::getUsername() const {
    return m_username;
}

void ClientSocket::send(const std::string& username, const std::string& message) {
    std::string formattedMessage = username + ": " + message;
    int bytes = ::send(m_socket, formattedMessage.c_str(), formattedMessage.length(), 0);
    if (bytes <= 0) {
        throw std::runtime_error("Failed to send data");
    }
}

bool ClientSocket::receive(std::string& _message) {
    char buffer[128] = { 0 };
    int bytes = ::recv(m_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes == SOCKET_ERROR) {
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
            m_closed = true;
            return false;
        }
        return false;
    }
    else if (bytes == 0) {
        m_closed = true;
        return false;
    }
    _message = buffer;
    return true;
}

bool ClientSocket::closed() {
    return m_closed;
}
