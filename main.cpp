#include <iostream>
#include <iomanip>
#include "class.hpp"

using std::cin;
using std::cerr;
using std::endl;

#include <arpa/inet.h>  // inet_addr()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h> // socket(), accept(), listen(), bind(), connect(), recv()
#include <sys/time.h>   // struct timespec
#include <fcntl.h>  // fcntl()
#include <sys/event.h>  // struct kevent, kevent()
#include <unistd.h>

void test(char** argv);
int serverSocket();
int serverBind(int serverSock, int portNumber);
void serverAccept(int serverSock, int kqueueFD);
void runRequest(const std::string requestMessage, std::string& responseMessage, int kqueueFD);
void describeStringAsHex(const std::string& string);
void describeSock(int sock);
void describeSockOpt(int sock, int optionName);




int main(int argc, char** argv) {
    if (argc != 2) {
        cerr << "usage: " << argv[0] << " <port number>" << endl;
        return 0;
    }

    test(argv);

    return 0;
}



void test(char** argv) {
    Config config = Config(NULL, argv);

    ServerManager serverManager(config);
    serverManager.eventLoop();
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
}

void describeSockOpt(int sock, int optionName) {
    int value = -1;
    socklen_t size = sizeof(value);

    getsockopt(sock, SOL_SOCKET, optionName, &value, &size);
    cerr << "optionName: " << optionName << ", value: " << value << ", size: " << size << endl;
}
