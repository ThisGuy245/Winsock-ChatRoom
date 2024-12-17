#include <WS2tcpip.h>
#include <stdexcept>
#include <string>
#include "ServerSocket.h"
#include "ClientSocket.h"


ServerSocket::ServerSocket(int _port) : m_socket(INVALID_SOCKET)
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		throw std::runtime_error("WSAStartup failed");
	}

	addrinfo hints = { 0 };
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	addrinfo* result = NULL;

	//server handling 
	if (getaddrinfo(NULL, std::to_string(_port).c_str(), &hints, &result) != 0)
	{
		throw std::runtime_error("Failed to resolve server address or port");
	}

	m_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	
	if (m_socket == INVALID_SOCKET)
	{
		freeaddrinfo(result);
		throw std::runtime_error("Failed to create socket");
	}

	if (bind(m_socket, result->ai_addr, result->ai_addrlen) == SOCKET_ERROR)
	{
		freeaddrinfo(result);
		throw std::runtime_error("Failed to bind socket");
	}

	freeaddrinfo(result);

	if (listen(m_socket, SOMAXCONN) == SOCKET_ERROR)
	{
		throw std::runtime_error("Failed to listen on socket");
	}
	
	u_long mode = 1;

	if (ioctlsocket(m_socket, FIONBIO, &mode) == SOCKET_ERROR)
	{
		throw std::runtime_error("Failed to set non-blocking");
	}
}

ServerSocket::~ServerSocket()
{
	closesocket(m_socket);
}

std::shared_ptr<ClientSocket> ServerSocket::accept()
{
	SOCKET socket = ::accept(m_socket, NULL, NULL);

	if (socket == INVALID_SOCKET)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			throw std::runtime_error("Failed to accept socket");
		}
		return nullptr;
	}

	//return a new ClientSocket made with a valid socket
	std::shared_ptr<ClientSocket> rtn = std::make_shared<ClientSocket>(socket);
	rtn->m_socket = socket;
	return rtn;
}

//needs to be on tick constatnly running
void ServerSocket::handleClientConnections() {
	// Accept new client connections
	std::shared_ptr<ClientSocket> client = accept();
	if (client) {
		printf("Client Connected!\n");

		// Request username from the client
		std::string username;
		if (client->receive(username)) {
			client->setUsername(username);
			printf("Username received: %s\n", username.c_str());

			std::string joinMessage = username + " has joined the server";
			for (size_t j = 0; j < clients.size(); ++j) {
				clients[j]->send(joinMessage);
			}
		}
		else {
			printf("Failed to receive username from client.\n");
		}

		// Add new client to the list
		clients.push_back(client);
	}

	// Handle messages from connected clients
	for (size_t i = 0; i < clients.size(); ++i) {
		std::string message;

		if (clients[i]->receive(message)) {
			const std::string& username = clients[i]->getUsername();
			printf("Message received from %s: %s\n", username.c_str(), message.c_str());

			// Broadcast the message to all clients
			std::string broadcastMessage = username + ": " + message;
			for (size_t j = 0; j < clients.size(); ++j) {
				if (i != j) {
					clients[j]->send(broadcastMessage);
				}
			}
		}
		else if (clients[i]->closed()) {
			// Client disconnected, send "has disconnected" message to others
			const std::string& username = clients[i]->getUsername();
			std::string disconnectMessage = username + " has disconnected";

			// Broadcast to other clients
			for (size_t j = 0; j < clients.size(); ++j) {
				if (i != j) {
					clients[j]->send(disconnectMessage);
				}
			}

			// Remove client from the list
			printf("%s has disconnected\n", username.c_str());
			clients.erase(clients.begin() + i);
			i--;  // Adjust the index after removal to avoid skipping a client
		}
	}
}