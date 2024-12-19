#ifndef SERVERSOCKET_H
#define SERVERSOCKET_H

#include <winsock2.h>
#include <memory>
#include <vector>
#include <string>
#include "ClientSocket.h"
#include "PlayerDisplay.hpp"  // Include the header where PlayerDisplay is declared

class ServerSocket
{
public:
    /**
     * @brief Constructor that initializes the server socket.
     * @param _port The port number on which the server will listen.
     * @param _playerDisplay A pointer to the PlayerDisplay instance.
     */
    ServerSocket(int _port, PlayerDisplay* _playerDisplay);

    /** Destructor that cleans up server resources. */
    ~ServerSocket();

    /**
     * @brief Accepts a new client connection and returns a shared pointer to the new client socket.
     * @return A shared pointer to the accepted ClientSocket.
     */
    std::shared_ptr<ClientSocket> accept();

    /** Closes all client connections. */
    void closeAllClients();

    /**
     * @brief Broadcasts a message to all connected clients.
     * @param message The message to be broadcast.
     */
    void broadcastMessage(const std::string& message);

    /** Handles all client connections and incoming messages. */
    void handleClientConnections();

    PlayerDisplay* playerDisplay; ///< Pointer to the PlayerDisplay instance

private:
    SOCKET m_socket;  ///< Main server socket
    std::vector<std::shared_ptr<ClientSocket>> clients; ///< Stores connected clients

    // Disable copying and assignment
    ServerSocket(const ServerSocket& _copy) = delete;
    ServerSocket& operator=(const ServerSocket& _assign) = delete;
};

#endif // SERVERSOCKET_H
