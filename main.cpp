#include <iostream>
#include <iomanip>
#include "message.hpp"

using std::cin;
using std::cerr;
using std::endl;

#include <arpa/inet.h>	// inet_addr()
#include <netinet/in.h>	// struct sockaddr_in
#include <sys/socket.h>	// socket(), accept(), listen(), bind(), connect(), recv()
#include <sys/time.h>	// struct timespec
#include <fcntl.h>	// fcntl()
#include <sys/event.h>	// struct kevent, kevent()
#include <unistd.h>

void test(int portNumber);
int socketServer();
int bindServer(int server_socket, int portNumber);
void listenServer(int server_socket);
void acceptServer(int server_socket, int kqueue_fd);
void addEventClient(int sock, int kqueue_fd);
void deleteEventClient(int sock, int kqueue_fd);
void modifyEventClient(int sock, int kqueue_fd, uint16_t flags);
void modifyEvent(int client_socket, int kqueue_fd, int16_t filter, uint16_t flags);
ssize_t readClient(int client_socket, char* msg_buf, std::size_t buf_size);
void runRequest(const std::string request_message, std::string& respons_message, bool& should_write);
void describeStringAsHex(const std::string& string);
void describeSock(int sock);
void describeSockOpt(int sock, int optionName);

const std::size_t DEFAULT_EVENT_ARRAY_SIZE = 32;
const std::size_t BUF_SIZE = 1024;

int main(int argc, char** argv) {
	if (argc != 2) {
		cerr << "usage: " << argv[0] << " <port number>" << endl;
		return 0;
	}

	test(atoi(argv[1]));

	return 0;
}

void test(int portNumber) {
	int server_socket = socketServer();
	cerr << "serverSocket: " << server_socket << endl;
	if (bindServer(server_socket, portNumber) == -1)
		return;
	listenServer(server_socket);

	int kqueue_fd = kqueue();
	modifyEvent(server_socket, kqueue_fd, EVFILT_READ, EV_ADD);

	int event_count;
	struct kevent event_array[DEFAULT_EVENT_ARRAY_SIZE];
	std::string response_message_array[DEFAULT_EVENT_ARRAY_SIZE];
	bool response_message_should_write[DEFAULT_EVENT_ARRAY_SIZE] = {};
	struct timespec timeout = { 0, 0 };
	char msg_buf[BUF_SIZE];
	fcntl(0, F_SETFL, O_NONBLOCK);
	while (cin.get() != 'q') {
		cin.clear();
		event_count = kevent(kqueue_fd, NULL, 0, event_array, DEFAULT_EVENT_ARRAY_SIZE, &timeout);
		if (event_count != 0)
			cerr << endl << "eventCount: " << event_count << endl;
		for (int i = 0; i < event_count; ++i) {
			const struct kevent& event = event_array[i];
			const int fd = event.ident;
			describeSock(fd);
			cerr << "fd: " << fd << ", event.filter: " << event.filter << endl;
			if (event.filter == EVFILT_READ) {
				if (static_cast<int>(fd) == server_socket)
					acceptServer(server_socket, kqueue_fd);
				else
					if (readClient(fd, msg_buf, BUF_SIZE) != 0)
						runRequest(msg_buf, response_message_array[fd], response_message_should_write[fd]);
					else {
						cerr << "close " << fd << endl;
						deleteEventClient(fd, kqueue_fd);
					}
			}
			else if (event.filter == EVFILT_WRITE)
				if (response_message_should_write[fd]) {
					const std::string response_message = response_message_array[fd];
					const size_t sendResult = send(fd, response_message.c_str(), response_message.length(), 0);
					describeSock(fd);
					cerr << "errno: " << errno << endl;
					cerr << "responseMessage.length(): " << response_message.length() << endl;
					cerr << "sendResult: " << sendResult << endl;
					response_message_should_write[fd] = false;
				}
		}
		usleep(100000);
	}

	close(server_socket);
}

int socketServer() {
	int server_socket = socket(PF_INET, SOCK_STREAM, 0);

	fcntl(server_socket, F_SETFL, O_NONBLOCK);

	return server_socket;
}

int bindServer(int server_socket, int portNumber) {
	struct sockaddr_in server_socket_address;
	const size_t size_of_server_socket_address = sizeof(server_socket_address);

	memset(&server_socket_address, 0, size_of_server_socket_address);
	server_socket_address.sin_family = AF_INET;
	server_socket_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_socket_address.sin_port = htons(portNumber);

	if (bind(server_socket, (struct sockaddr*) &server_socket_address, size_of_server_socket_address) == -1) {
		cerr << "fail bind()" << endl;

		return -1;
	}

	cerr << "success bind()" << endl;

	return 0;
}

void listenServer(int server_socket) {
	listen(server_socket, 5);
}

void acceptServer(int server_socket, int kqueue_fd) {
	struct sockaddr_in client_socket_address;
	socklen_t client_socket_address_size = sizeof(client_socket_address);
	int client_socket = accept(server_socket, (struct sockaddr*)&client_socket_address, &client_socket_address_size);
	cerr << "clientSocket: " << client_socket << endl;
	addEventClient(client_socket, kqueue_fd);
}

// event
void addEventClient(int sock, int kqueue_fd) {
	modifyEventClient(sock, kqueue_fd, EV_ADD);
}

void deleteEventClient(int sock, int kqueue_fd) {
	modifyEventClient(sock, kqueue_fd, EV_DELETE);
	close(sock);
}

void modifyEventClient(int sock, int kqueue_fd, uint16_t flags) {
	modifyEvent(sock, kqueue_fd, EVFILT_READ, flags);
	modifyEvent(sock, kqueue_fd, EVFILT_WRITE, flags);
}

void modifyEvent(int sock, int kqueue_fd, int16_t filter, uint16_t flags) {
	struct kevent event;
	EV_SET(&event, sock, filter, flags, 0, 0, NULL);
	kevent(kqueue_fd, &event, 1, NULL, 0, NULL);
}
// event

ssize_t readClient(int sock, char* msg_buf, std::size_t buf_size) {
	const ssize_t count_of_received_bytes = recv(sock, msg_buf, buf_size - 1, 0);
	msg_buf[count_of_received_bytes] = '\0';
	return count_of_received_bytes;
}

void runRequest(const std::string request_message, std::string& response_message, bool& should_write) {
	HTTP::Worker worker;
	HTTP::Response response;

	HTTP::Response::initStatusCodeMap(NULL);

	// print request message in string
// 	cerr << "< request_message >" << endl;
// 	cerr << "[" << request_message << "]" << endl;
// 	cerr << endl;

	// print request message in hex
// 	cerr << "< request_message in hex >" << endl;
// 	cerr << "[";
// 	describeStringAsHex(request_message);
// 	cerr << "]" << endl;
// 	cerr << endl;

	// print request
	HTTP::Request request = HTTP::Request(request_message);
// 	cerr << "< request describe >" << endl;
// 	request.describe(cerr);
// 	cerr << endl;

	if (worker.runRequest(request, response) == -1) {
		cerr << "something wrong" << endl;
		return;
	}

	// print response
// 	cerr << "< response describe >" << endl;
// 	response.describe(cerr);
// 	cerr << endl;

	// print response message in string
	response_message = response.convertToString();
// 	cerr << "< response message >" << endl;
// 	cerr << response_message << endl;

	should_write = true;
}

void describeStringAsHex(const std::string& string) {
	cerr << std::hex << std::setfill('0');
	int index = 0;
	for (std::string::const_iterator it = string.begin(); it != string.end(); ++it) {
		if (index % 2 == 0)
			cerr << ' ';
		if (index % 8 == 0)
			cerr << ' ';
		if (index % 16 == 0)
			cerr << '\n';

		cerr << std::setw(2) << (int)*it;
		++index;
	}
	cerr << std::dec;
}

void describeSock(int sock) {
	describeSockOpt(sock, SO_NWRITE);
	describeSockOpt(sock, SO_SNDBUF);
// 	describeSockOpt(sock, SO_ERROR);
}

void describeSockOpt(int sock, int optionName) {
	int value = -1;
	socklen_t size = sizeof(value);

	getsockopt(sock, SOL_SOCKET, optionName, &value, &size);
	cerr << "optionName: " << optionName << ", value: " << value << ", size: " << size << endl;
}
