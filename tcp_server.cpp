#include "tcp_server.h"

SERVER_TCP::SERVER_TCP(const std::string localaddr, const int port, net_callback callback)
	: local_ipv4_addr(localaddr), local_port(port), intern_callback(callback) {
};


SOCKET SERVER_TCP::createSocket(){
	SOCKET listener = socket(AF_INET, SOCK_STREAM, 0);
	if (listener != INVALID_SOCKET) {
		sockaddr_in hints;
		hints.sin_family = AF_INET;
		hints.sin_port = htons(local_port);
		inet_pton(AF_INET,local_ipv4_addr.c_str(),&hints.sin_addr);
		if (bind(listener, reinterpret_cast<sockaddr*>(&hints), sizeof(hints)) == SOCKET_ERROR || listen(listener, SOMAXCONN) == SOCKET_ERROR) return INVALID_SOCKET;
		return listener;
	}
	return INVALID_SOCKET;
}

SOCKET SERVER_TCP::awaitSocket(SOCKET listener) {
	fd_set readSet;
	FD_ZERO(&readSet);
	FD_SET(listener, &readSet);
	timeval timeout;
	timeout.tv_sec = 1;  // Zero timeout (poll)
	timeout.tv_usec = 0;
	if (select(listener, &readSet, NULL, NULL, &timeout) == 1) return accept(listener, 0, 0);
	return INVALID_SOCKET;
}
void SERVER_TCP::sendResponse(int socket, std::string &data){send(socket, data.c_str(),data.size()+1,0);}

void SERVER_TCP::startServer() {
	for (;;) {
		SOCKET listener = createSocket();
		if (listener == INVALID_SOCKET) throw std::runtime_error{"SocketInvalid"};
		SOCKET intercom = awaitSocket(listener);
		closesocket(listener);
		if (intercom != INVALID_SOCKET) {
			size_t recieved_bytes = 0;
			do {
				char buf[4096] = { '\0' };
				recieved_bytes = recv(intercom, buf, 4096, 0);
				if (recieved_bytes > 0 && intern_callback != nullptr) {
					try { intern_callback(this, intercom, std::make_pair(callbackarg, std::string(buf, 0, 4096)));}
					catch (std::exception e) {
						closesocket(intercom);
						std::rethrow_exception(std::make_exception_ptr(e));
					}
				}
			} while (recieved_bytes > 0);
			closesocket(intercom);
		}
	}
}

void SERVER_TCP::customizeCallback(void* newarg) {callbackarg = newarg;}