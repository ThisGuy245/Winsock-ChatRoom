// ServerSocket.h
#ifndef SERVER_SOCKET_H
#define SERVER_SOCKET_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <memory>

class ClientSocket;

class ServerSocket {
private:
    SOCKET serverSocket;
    std::string serverIP;
    std::vector<std::shared_ptr<ClientSocket>> clients;
    std::vector<std::string> usernames;
    std::vector<std::string> getConnectedUsers() const {
    
    }

    void configureSocket(int port);
    void retrieveHostIP();

public:
    explicit ServerSocket(int port);
    ~ServerSocket();

    std::shared_ptr<ClientSocket> acceptClient();
    void manageClients();
    std::string getServerIP() const;
};

#endif // SERVER_SOCKET_H