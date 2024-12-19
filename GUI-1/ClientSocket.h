#ifndef CLIENTSOCKET_H
#define CLIENTSOCKET_H

#include <string>
#include <stdexcept>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "PlayerDisplay.hpp" // Include PlayerDisplay header

class ClientSocket {
public:
    // Constructors
    explicit ClientSocket(SOCKET socket, PlayerDisplay* playerDisplay);  // Pointer to PlayerDisplay
    ClientSocket(const std::string& ipAddress, int port, const std::string& username, PlayerDisplay* playerDisplay);

    // Destructor
    ~ClientSocket();

    // Methods
    void setUsername(const std::string& username);
    const std::string& getUsername() const;
    void send(const std::string& message);
    bool receive(std::string& message);
    void changeUsername(const std::string& newUsername);
    void addingPlayer(const std::string& username);
    void removingPlayer(const std::string& username);
    bool closed();

private:
    SOCKET m_socket;
    bool m_closed;
    std::string m_username;
    PlayerDisplay* playerDisplay;  // Pointer to PlayerDisplay (not a reference)
};

#endif // CLIENTSOCKET_H
