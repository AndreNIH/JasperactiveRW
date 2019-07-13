#pragma once
#include <utility>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdexcept>
#include <string>


class SERVER_TCP;
typedef void(*net_callback)(SERVER_TCP* listener, int sock, std::pair<void*,std::string>(data));

class SERVER_TCP {
private:
	const std::string local_ipv4_addr;
	const int local_port;
	net_callback intern_callback = nullptr;

	SOCKET createSocket();
	SOCKET awaitSocket(SOCKET listener);

	void* callbackarg = nullptr;

public:
	SERVER_TCP(const std::string localaddr, int port, net_callback callback);
	void sendResponse(int socket, std::string &data);
	void startServer();
	void customizeCallback(void* newarg);
};