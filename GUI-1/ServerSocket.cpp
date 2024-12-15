
#include <WS2tcpip.h>
#include <stdexcept>
#include <string>
#include "ServerSocket.h"

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
void ServerSocket::handleClientConnections()
{
	// Accept new client connections
	std::shared_ptr<ClientSocket> client = accept();
	if (client)
	{
		printf("Client Connected!\n");
		clients.push_back(client);
	}

	// Handle messages from connected clients
	for (int i = 0; i < clients.size(); ++i)
	{
		std::string message;
		bool receivedMessage = clients[i]->receive(message);

		if (receivedMessage)
		{
			printf("Message received: %s\n", message.c_str());
			for (int i = 0; i < clients.size(); i++)
			{
				clients[i]->send(message);
			}
		}

		//if (!receivedMessage)
		//{
		//	printf("Client Disconnected\n");
		//	std::string number = std::to_string(i);
		//	printf(number.c_str());

		//	//gets rid of client from list
		//	clients.erase(clients.begin() + i);
		//	--i;
		//}
	}
}

