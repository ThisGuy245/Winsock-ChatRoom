#ifndef CLIENTSOCKET_H
#define CLIENTSOCKET_H

#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

class ClientSocket {
public:
    // Constructor for server-side socket
    explicit ClientSocket(SOCKET sock);

    // Constructor for client-side connection
    ClientSocket(const std::string& serverIP, int serverPort);

    // Destructor
    ~ClientSocket();

    // Public methods
    void sendMessage(const std::string& tag, const std::string& message);
    bool receiveMessage(std::string& tag, std::string& message);
    bool isConnectionClosed() const;

private:
    void configureSocket(const std::string& serverIP, int serverPort);

    SOCKET clientSocket; // Holds the socket for communication
    bool closed;         // Tracks if the socket is closed
};

#endif
