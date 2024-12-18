#ifndef SERVERSOCKET_H
#define SERVERSOCKET_H

#include <WinSock2.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include "ClientSocket.h"

/**
 * @class ServerSocket
 * @brief Manages server-side networking, handling client connections and broadcasting messages.
 */
class ServerSocket {
public:
    /**
     * @brief Constructs a ServerSocket bound to the specified port.
     * @param port The port number to bind the server socket.
     */
    ServerSocket(int port);

    /**
     * @brief Destructor for ServerSocket. Closes the socket and cleans up resources.
     */
    ~ServerSocket();

    /**
     * @brief Accepts a new client connection.
     * @return A shared pointer to the connected ClientSocket instance.
     */
    std::shared_ptr<ClientSocket> accept();

    /**
     * @brief Handles client connections, to be periodically called (e.g., using a timer).
     */
    void handleClientConnections();

    /**
     * @brief Broadcasts a message to all connected clients except the specified one.
     * @param message The message to broadcast.
     * @param excludeClient A shared pointer to the client to exclude from the broadcast (default is nullptr).
     */
    void broadcastMessage(const std::string& message, std::shared_ptr<ClientSocket> excludeClient = nullptr);

private:
    SOCKET m_socket;                                   /**< The server socket instance. */
    std::vector<std::shared_ptr<ClientSocket>> clients; /**< List of currently connected clients. */
    std::unordered_map<std::shared_ptr<ClientSocket>, std::string> clientUsernames; /**< Map of clients to their usernames. */
};

#endif // SERVERSOCKET_H
