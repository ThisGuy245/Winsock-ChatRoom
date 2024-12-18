#pragma once
#include <winsock2.h>
#include <string>
#include <memory>
#include <stdexcept>
#include <WS2tcpip.h>

/**
 * @brief Represents a client-side socket for communication with a server or peer.
 */
struct ClientSocket {
    /**
     * @brief Constructs a ClientSocket instance from an existing socket.
     * @param socket The existing socket descriptor.
     */
    ClientSocket(SOCKET socket);

    /**
     * @brief Constructs a ClientSocket instance and connects to a server.
     * @param ipAddress The server's IP address.
     * @param port The server's port number.
     */
    ClientSocket(const std::string& ipAddress, int port);

    /**
     * @brief Destructor that cleans up the socket resources.
     */
    ~ClientSocket();

    /**
     * @brief Receives a message from the socket.
     * @param sender The variable to store the sender's identifier.
     * @param _message The variable to store the received message.
     * @return True if the message was received successfully, false otherwise.
     */
    bool receive(std::string& sender, std::string& _message);

    /**
     * @brief Sends a message to the connected peer.
     * @param username The username of the sender.
     * @param _message The message to be sent.
     */
    void send(const std::string& username, const std::string& _message);

    /**
     * @brief Checks if the socket has been closed.
     * @return True if the socket is closed, false otherwise.
     */
    bool closed();

private:
    friend struct ServerSocket; ///< Grants ServerSocket access to private members.

    SOCKET m_socket; ///< The underlying socket descriptor.
    bool m_closed; ///< Indicates if the socket is closed.

    // Prevent copying
    ClientSocket(const ClientSocket& _copy);
    ClientSocket& operator=(const ClientSocket& _assign);
};
