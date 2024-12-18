#pragma once
#include <winsock2.h>
#include <string>
#include <memory>
#include <stdexcept>
#include <WS2tcpip.h>

struct ServerSocket;

struct ClientSocket {
    ClientSocket(SOCKET socket);
    ClientSocket(const std::string& ipAddress, int port, const std::string& username);
    ~ClientSocket();

    void setUsername(const std::string& username);
    const std::string& getUsername() const;

    void changeUsername(const std::string& newUsername);

    void send(const std::string& message);
    bool receive(std::string& message);

    bool closed();

private:
    friend struct ServerSocket;
    SOCKET m_socket;
    bool m_closed;
    std::string m_username;

    ClientSocket(const ClientSocket& _copy) = delete;
    ClientSocket& operator=(const ClientSocket& _assign) = delete;
};
