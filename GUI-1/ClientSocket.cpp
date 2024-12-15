#include "ClientSocket.h"
#include "ServerSocket.h"

//this is for already sockets
ClientSocket::ClientSocket(SOCKET socket) : m_socket(socket), m_closed(false)
{
	if (socket == INVALID_SOCKET)
	{
		throw std::runtime_error("Invalid socket");
	}
}

//this is for new ClientSockets
ClientSocket::ClientSocket(const std::string& ipAddress, int port) : m_socket(INVALID_SOCKET), m_closed(false)
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		throw std::runtime_error("WSAStartup failed");
	}

	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_socket == INVALID_SOCKET)
	{
		WSACleanup();
		throw std::runtime_error("Failed to create socket");
	}

	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(port);

	//convert IP address using inet_pton
	if (inet_pton(AF_INET, ipAddress.c_str(), &serverAddress.sin_addr) <= 0)
	{
		closesocket(m_socket);
		WSACleanup();
		throw std::runtime_error("Invalid address / Address not supported");
	}

	if (connect(m_socket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR)
	{
		closesocket(m_socket);
		WSACleanup();
		throw std::runtime_error("Failed to connect to server");
	}

	u_long mode = 1;

	if (ioctlsocket(m_socket, FIONBIO, &mode) == SOCKET_ERROR)
	{
		throw std::runtime_error("Failed to set non-blocking");
	}

	printf("Connected to server at %s:%d\n", ipAddress.c_str(), port);
}

ClientSocket::~ClientSocket()
{
	if (m_socket != INVALID_SOCKET)
	{
		closesocket(m_socket);
	}
}

//to accept a message from client socket
bool ClientSocket::receive(std::string& _message)
{
	char buffer[128] = { 0 };
	int bytes = ::recv(m_socket, buffer, sizeof(buffer) - 1, 0);
	if (bytes == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			m_closed = true;
			return false;
			//throw std::runtime_error("Read failed");
		}
		return false;
	}
	else if (bytes == 0)
	{
		m_closed = true;
		return false;
	}
	_message = buffer;
	return true;
}

//to send a message to client socket
void ClientSocket::send(const std::string& _message)
{
	int bytes = ::send(m_socket, _message.c_str(), _message.length(), 0);
	if (bytes <= 0)
	{
		throw std::runtime_error("Failed to send data");
	}
}

bool ClientSocket::closed()
{
	return m_closed;
}