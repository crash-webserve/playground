#include <iostream>
#include <iomanip>
#include "message.hpp"

using std::cout;
using std::endl;

void testSocketRequest();
std::string recieveRequest();
void describeStringAsHex(const std::string& string);

void testSocketResponse();

int main() {
	testSocketRequest();
	cout << endl;

	testSocketResponse();
}

void testSimpleRequest() {
	std::string request_message = "GET /index.html HTTP/1.1\n Host: localhost\n\n";
	HTTP::Request request = HTTP::Request(request_message);
	request.describe();
}

#include <arpa/inet.h>	// inet_addr()
#include <netinet/in.h>	// struct sockaddr_in
#include <sys/socket.h>	// socket(), accept(), listen(), bind(), connect(), recv()
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/event.h>

void testSocketRequest() {
	std::string request_message = recieveRequest();
	cout << endl;

	cout << "< request_message >" << endl;
	cout << "[" << request_message << "]" << endl;
	cout << endl;

	cout << "< request_message in hex >" << endl;
	cout << "[";
	describeStringAsHex(request_message);
	cout << "]" << endl;
	cout << endl;

	HTTP::Request request = HTTP::Request(request_message);
	cout << "< request describe >" << endl;
	request.describe();
}

std::string recieveRequest() {
		//	socket()
	int serverSocket = socket(PF_INET, SOCK_STREAM, 0);
	if (serverSocket == -1) {
		cout << "fail socket()" << endl;
		return std::string();
	}
	else
		cout << "success socket()" << endl;
// 	fcntl(serverSocket, F_SETFL, O_NONBLOCK);

	//	bind()
	struct sockaddr_in serverSocketAddress;
	memset(&serverSocketAddress, 0, sizeof(serverSocketAddress));
	serverSocketAddress.sin_family = AF_INET;
	serverSocketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverSocketAddress.sin_port = htons(atoi("80"));

	if (bind(serverSocket, (struct sockaddr*) &serverSocketAddress, sizeof(serverSocketAddress)) == -1) {
		cout << "fail bind()" << endl;
		return std::string();
	}
	else
		cout << "success bind()" << endl;

	//	listen()
	if (listen(serverSocket, 5) == -1) {
		cout << "fail listen()" << endl;
		return std::string();
	}
	else
		cout << "success listen()" << endl;

	int clientSocket;
	struct sockaddr_in clientSocketAddress;
	socklen_t clientSocketAddressSize = sizeof(clientSocketAddressSize);
	struct kevent kev;
	int kqueueFD = kqueue();
	struct kevent keventArray[32];
	int eventCount;
	char msgBuffer[1024];

// 	struct timespec timeout = {};
// 	timeout.tv_nsec = 1;

	clientSocket = accept(serverSocket, (struct sockaddr*)&clientSocketAddress, &clientSocketAddressSize);
	if (clientSocket == -1) {
		cout << "fail accept()" << endl;
		return std::string();
	}
	else {
		cout << "success accept()" << endl;
		EV_SET(&kev, clientSocket, EVFILT_READ, EV_ADD, 0, 0, NULL);
		kevent(kqueueFD, &kev, 1, NULL, 0, NULL);
	}

	std::string requestMessage;
	ssize_t countOfReceivedBytes;
	while (true) {
		eventCount = kevent(kqueueFD, NULL, 0, keventArray, 32, NULL);
		if (eventCount == 0)
			continue;
		countOfReceivedBytes = recv(keventArray[0].ident, msgBuffer, 1024, 0);
		if (countOfReceivedBytes != 0) {
			msgBuffer[countOfReceivedBytes] = '\0';
			requestMessage += msgBuffer;
		}
		else if (countOfReceivedBytes == 0)
			break;
	}

	return requestMessage;
}

void testSocketResponse() {
	Server server;
	HTTP::Response response;

// 	std::string request_message = recieveRequest();
// 	HTTP::Request request(request_message);
	HTTP::Request request("GET / HTTP/1.1");
	if (server.runRequest(request, response) == -1) {
		cout << "something wrong" << endl;
		return;
	}

	cout << "< response describe >" << endl;
	response.describe();
	cout << endl;

	std::string response_message = response.convertToString();

	cout << "< response message >" << endl;
	cout << response_message << endl;
}

void describeStringAsHex(const std::string& string) {
	cout << std::hex << std::setfill('0');
	int index = 0;
	for (std::string::const_iterator it = string.begin(); it != string.end(); ++it) {
		if (index % 2 == 0)
			cout << ' ';
		if (index % 8 == 0)
			cout << ' ';
		if (index % 16 == 0)
			cout << '\n';

		cout << std::setw(2) << (int)*it;
		++index;
	}
	cout << std::dec;
}
