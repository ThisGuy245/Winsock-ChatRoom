#pragma once
#include <winsock2.h>
#include <string>
#include <memory>
#include <stdexcept>
#include <WS2tcpip.h>


struct ServerSocket;

struct ClientSocket
{
	ClientSocket(SOCKET socket);
	ClientSocket(const std::string& ipAddress, int port);
	~ClientSocket();

	bool receive(std::string& sender, std::string& _message);
	void send(const std::string& username, const std::string& _message);
	bool closed();


private:
	friend struct ServerSocket;
	SOCKET m_socket;
	bool m_closed;
	ClientSocket(const ClientSocket& _copy);
	ClientSocket& operator=(const ClientSocket& _assign);

};