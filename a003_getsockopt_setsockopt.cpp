#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

using std::cin;
using std::cout;
using std::endl;

static void lmi_getline(std::istream& istream, std::string& line);
static void print16Bytes(const std::string& bytes);
static void sockaddr_in_describe(struct sockaddr_in& socketAddress);

const int BUF_SIZE = 8192;

template <typename T, typename U>
struct Pair {
    T command;
    U method;
};

class Program {
private:
    typedef void* (Program::*Method)(void);
    typedef Pair<const char*, Method> MethodPair;

    uint16_t portNumber;
    int serverSocketFD;
    int clientSocketFD;

    int remoteServerSocketFD;
	struct sockaddr_in remoteServerSocketAddress;

    static void describeMethodCommand(void);

    void* describe(void);
    void* setPort(void);
    void* listen(void);
    void* acceptClient(void);
    void* describeSocketOption(void);
    void* sendBigString(void);
    void* closeServerSocket(void);
    void* closeClientSocket(void);
    void* close(void);
    void* recv(void);
    void* send(void);
    void* connectRemote(void);
    void* sendRemote(void);
    void* recvRemote(void);
    void* closeRemote(void);
    void* reconnectRemote(void);

    static const MethodPair methodDictionary[];

public:
    Program(void)
        : portNumber(0)
        , serverSocketFD(-1)
        , clientSocketFD(-1)
        , remoteServerSocketFD(-1)
        , remoteServerSocketAddress()
    { };

    void mainLoop(void);
};

const Program::MethodPair Program::methodDictionary[] = {
    { "describe", &Program::describe },
    { "set port", &Program::setPort },
    { "listen", &Program::listen },
    { "accept", &Program::acceptClient },
    { "describe socket", &Program::describeSocketOption },
    { "send big string", &Program::sendBigString },
    { "close server socket", &Program::closeServerSocket },
    { "close client socket", &Program::closeClientSocket },
    { "close", &Program::close },
    { "recv", &Program::recv },
    { "send", &Program::send },
    { "connect remote", &Program::connectRemote },
    { "send remote", &Program::sendRemote },
    { "recv remote", &Program::recvRemote },
    { "close remote", &Program::closeRemote },
    { "reconnect remote", &Program::reconnectRemote },
};

void* Program::describe(void) {
    cout << "port number: " << this->portNumber << endl;
    cout << "server socket fd: " << this->serverSocketFD << endl;
    cout << "client socket fd: " << this->clientSocketFD << endl;
    cout << endl;
    cout << "this->remoteServerSocketFD: "<< this->remoteServerSocketFD << endl;
    sockaddr_in_describe(this->remoteServerSocketAddress);

    return NULL;
}

void* Program::setPort(void) {
    cout << "current port number: " << this->portNumber << endl;
    cout << "enter new port number: ";
    std::string line;
    lmi_getline(cin, line);
    std::istringstream inputStringStream(line);
    inputStringStream >> this->portNumber;
    if (!inputStringStream)
        throw "fail getting port number";
    else
        cout << "port number: " << this->portNumber << endl;

    return NULL;
}

void* Program::listen(void) {
    this->setPort();

    this->serverSocketFD = socket(PF_INET, SOCK_STREAM, 0);
    fcntl(this->serverSocketFD, F_SETFL, O_NONBLOCK);

    struct sockaddr_in serverSocketAddress;
    memset(&serverSocketAddress, 0, sizeof(serverSocketAddress));
    serverSocketAddress.sin_family = AF_INET;
    serverSocketAddress.sin_port = htons(this->portNumber);
    serverSocketAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(this->serverSocketFD, (struct sockaddr*)&serverSocketAddress, sizeof(serverSocketAddress)) == -1)
        throw "failed binding";

    ::listen(this->serverSocketFD, 5);

    cout << "listening " << this->portNumber << endl;

    return NULL;
}

void* Program::acceptClient(void) {
    if (this->clientSocketFD != -1)
        throw "No sit, sorry";

    struct sockaddr clientSocketAddress;
    socklen_t clientSocketAddressSize = sizeof(clientSocketAddress);
    this->clientSocketFD = accept(this->serverSocketFD, &clientSocketAddress, &clientSocketAddressSize);
    if (this->clientSocketFD == -1)
        cout << "fail accept" << endl;
    else
        cout << "success accept" << endl;

    return NULL;
}

void* Program::describeSocketOption(void) {
    int optionValue;
    socklen_t optionLength = sizeof(optionValue);

    getsockopt(this->clientSocketFD, SOL_SOCKET, SO_SNDBUF, &optionValue, &optionLength);

    cout << "SO_SNDBUF: " << optionValue << ", size: " << optionLength << endl;

    return NULL;
}

void* Program::sendBigString(void) {
    std::string bigString = std::string(10, '1');
    ssize_t sendSize = ::send(this->clientSocketFD, bigString.c_str(), 10, 0);
    if (sendSize == -1)
        cout << "send error has occured" << endl;
    else
        cout << "sended big string" << endl;

    return NULL;
}

void* Program::closeServerSocket(void) {
    ::close(this->serverSocketFD);
    this->serverSocketFD = -1;

    cout << "closed server socket" << endl;

    return NULL;
}

void* Program::closeClientSocket(void) {
    ::close(this->clientSocketFD);
    this->clientSocketFD = -1;

    cout << "closed client socket" << endl;

    return NULL;
}

void* Program::close(void) {
    this->closeClientSocket();
    this->closeServerSocket();

    return NULL;
}

void* Program::recv(void) {
    char buf[BUF_SIZE + 1];
    ssize_t recvSize = ::recv(this->clientSocketFD, buf, BUF_SIZE, 0);
    if (recvSize == -1)
        throw "recv error has occured";
    else if (recvSize == 0)
        throw "no data";

    buf[recvSize] = '\0';

    cout << "received size: " << recvSize << endl;
    for (int index = 0; index < recvSize; index += 16) {
        print16Bytes(std::string(buf + index, buf + index + 16));
    }

    return NULL;
}

void* Program::send(void) {
    cout << "enter line to send: ";
    std::string line;
    lmi_getline(cin, line);
    line += "\r\n";
    ssize_t result = ::send(this->clientSocketFD, line.c_str(), line.length(), 0);
    if (result <= 0)
        throw "result of send <= 0";
    cout << "succeeded send" << endl;

    return NULL;
}

void* Program::connectRemote(void) {
    int serverSocketFD;
	struct sockaddr_in serverSocketAddress;
    std::string serverInetAddressString;
    unsigned long serverInetAddress;
    std::string serverPortNumberString;
    uint16_t serverPortNumber;
    int result;

    serverSocketFD = socket(PF_INET, SOCK_STREAM, 0);
	if (serverSocketFD == -1)
		throw "fail socket()";

    cout << "enter inet address of server: ";
    lmi_getline(cin, serverInetAddressString);
    serverInetAddress = inet_addr(serverInetAddressString.c_str());

    cout << "enter port number of server: ";
    lmi_getline(cin, serverPortNumberString);
    serverPortNumber = htons(atoi(serverPortNumberString.c_str()));

	memset(&serverSocketAddress, 0, sizeof(serverSocketAddress));
	serverSocketAddress.sin_family = AF_INET;
	serverSocketAddress.sin_addr.s_addr = serverInetAddress;
	serverSocketAddress.sin_port = serverPortNumber;

	result = connect(serverSocketFD, (struct sockaddr*)&serverSocketAddress, sizeof(serverSocketAddress));
    fcntl(serverSocketFD, F_SETFL, O_NONBLOCK); // connect 전에 non-blocking으로 만들면 connect 에러가 발생한다 ㅎㅎ
    if (result == -1)
		throw "fail connect()";

    cout << "success connect()" << endl;

    this->remoteServerSocketFD = serverSocketFD;
    this->remoteServerSocketAddress = serverSocketAddress;

    return NULL;
}

void* Program::sendRemote(void) {
    cout << "enter line to send[exit: q]:" << endl;
    std::string requestMessage;
    while (true) {
        std::string line;
        lmi_getline(cin, line);
        if (line == "q")
            break;
        requestMessage += line + "\r\n";
    }

    ssize_t result = ::send(this->remoteServerSocketFD, requestMessage.c_str(), requestMessage.length(), 0);
    if (result <= 0)
        throw "result of send <= 0";
    cout << "succeeded send" << endl;

    return NULL;
}

void* Program::recvRemote(void) {
    char buf[BUF_SIZE + 1];
    ssize_t recvSize = ::recv(this->remoteServerSocketFD, buf, BUF_SIZE, 0);
    if (recvSize == -1)
        throw "recv error has occured";
    else if (recvSize == 0)
        throw "no data";

    buf[recvSize] = '\0';

    cout << "received size: " << recvSize << endl;
    for (int index = 0; index < recvSize; index += 16) {
        print16Bytes(std::string(buf + index, buf + index + 16));
    }

    return NULL;
}

void* Program::closeRemote(void) {
    if (this->remoteServerSocketFD == -1)
        throw "no remote server socket to close";
    ::close(this->remoteServerSocketFD);
    this->remoteServerSocketFD = -1;
    this->remoteServerSocketAddress = sockaddr_in();

    cout << "closed remote server socket" << endl;

    return NULL;
}

void* Program::reconnectRemote(void) {
    int serverSocketFD;
	struct sockaddr_in serverSocketAddress;
    std::string serverInetAddressString;
    std::string serverPortNumberString;
    int result;

    if (this->remoteServerSocketFD == -1)
        return this->connectRemote();

    serverSocketAddress = this->remoteServerSocketAddress;

    this->closeRemote();

    serverSocketFD = socket(PF_INET, SOCK_STREAM, 0);
	if (serverSocketFD == -1)
		throw "fail socket()";

	result = connect(serverSocketFD, (struct sockaddr*)&serverSocketAddress, sizeof(serverSocketAddress));
    fcntl(serverSocketFD, F_SETFL, O_NONBLOCK); // connect 전에 non-blocking으로 만들면 connect 에러가 발생한다 ㅎㅎ
    if (result == -1)
		throw "fail connect()";

    cout << "success reconnect()" << endl;

    this->remoteServerSocketFD = serverSocketFD;
    this->remoteServerSocketAddress = serverSocketAddress;

    return NULL;
}



// MARK: - program
void Program::mainLoop(void) {
    std::string line;

    while (true) {
        cout << endl << "enable command list:" << endl;
        Program::describeMethodCommand();
        cout << "--------------" << endl;
        cout << "enter command: ";

        lmi_getline(cin, line);

        unsigned long i;
        for (i = 0; i < sizeof(methodDictionary) / sizeof(MethodPair); ++i) {
            const MethodPair* pair = &methodDictionary[i];

            if (line == pair->command)
                break;
        }
        if (i == sizeof(methodDictionary) / sizeof(MethodPair)) {
            cout << "not found: [" << line << "]" << endl;
            continue;
        }
        try {
            const MethodPair* pair = &methodDictionary[i];
            (this->*(pair->method))();
        } catch(const char* string) {
            cout << line << ": " << string << endl;
        }
    }
}



// MARK: - static
void Program::describeMethodCommand(void) {
    for (unsigned long i = 0; i < sizeof(methodDictionary) / sizeof(MethodPair); ++i) {
        const MethodPair& pair = methodDictionary[i];

        cout << "\t" << pair.command << endl;
    }
}

static void lmi_getline(std::istream& istream, std::string& line) {
    std::getline(istream, line);
    if (!istream)
        throw "failed getline";
}

static void print16Bytes(const std::string& bytes) {
    for (int i = 0; i < 16; ++i) {
        if (i % 8 == 0)
            cout << "  ";
        else if (i % 2 == 0)
            cout << " ";
        cout << std::hex << std::setfill('0') << std::setw(2) << (int)(unsigned char)bytes[i];
    }

    cout << "   ";
    for (int i = 0; i < 16; ++i) {
        const char ch = bytes[i];
        cout << (std::isprint(ch) ? ch : '.');
    }

    cout << endl;
}

static void sockaddr_in_describe(struct sockaddr_in& socketAddress) {
	cout << "socketAddress.sin_len: " << socketAddress.sin_len << endl;
	cout << "socketAddress.sin_family: " << std::hex << std::setfill('0') << std::setw(2) << (int)(unsigned char)socketAddress.sin_family << endl;
	cout << "socketAddress.sin_port: " << socketAddress.sin_port << endl;
	cout << "socketAddress.sin_addr: " << socketAddress.sin_addr.s_addr << endl;
    std::string sin_zero = std::string(socketAddress.sin_zero, socketAddress.sin_zero + 7);
	cout << "socketAddress.sin_zero: " << sin_zero << endl;
    print16Bytes(sin_zero);
}



// MARK: - main
int main(void) {
    Program program;

    program.mainLoop();

    return 0;
}
