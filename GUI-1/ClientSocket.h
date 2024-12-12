#ifndef CLIENTSOCKET_H
#define CLIENTSOCKET_H

#include <string>
#include <winsock2.h>

struct ClientSocket {
public:
    // Constructor for creating an uninitialized client socket
    ClientSocket();

    // Constructor for wrapping an existing socket
    explicit ClientSocket(SOCKET socket);

    // Destructor
    ~ClientSocket();

    // Connect to a server at the given address and port
    bool ConnectToServer(const std::string& address, int port);
    bool isConnected() const;

    void receiveUserList(std::string& message);

    // Receive a message from the socket
    bool receive(std::string& message);

    // Send a message through the socket
    void send(const std::string& message);

    // Check if the socket is closed
    bool closed() const;

    // Close the socket manually
    void close();

    std::string getUsername() const;
    void setUsername(const std::string& username);

private:
    SOCKET m_socket;  // Underlying socket
    bool m_closed;    // Whether the socket is closed
    int m_playerId;   // Unique player ID for this client
    std::string m_username;
};

#endif // CLIENTSOCKET_H
