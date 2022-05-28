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

class Program {
private:
    typedef void (Program::*Method)(void);
    typedef std::pair<std::string, Method> MethodMapElementType;
    typedef std::map<std::string, Method> MethodMap;

    static const MethodMap methodMap;

    uint16_t portNumber;
    int serverSocketFD;
    int clientSocketFD;

    static void describeMethodCommand(void);

    void describe(void);
    void setPort(void);
    void listen(void);
    void acceptClient(void);
    void describeSocketOption(void);
    void sendBigString(void);
    void closeServerSocket(void);
    void closeClientSocket(void);
    void close(void);
    void recv(void);

public:
    Program(void): portNumber(0), serverSocketFD(-1), clientSocketFD(-1) { };

    void mainLoop(void);
};

const Program::MethodMap Program::methodMap = {
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
};

void Program::describe(void) {
    cout << "port number: " << this->portNumber << endl;
    cout << "server socket fd: " << this->serverSocketFD << endl;
    cout << "client socket fd: " << this->clientSocketFD << endl;
}

void Program::setPort(void) {
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
}

void Program::listen(void) {
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
}

void Program::acceptClient(void) {
    if (this->clientSocketFD != -1)
        throw "No sit, sorry";

    struct sockaddr clientSocketAddress;
    socklen_t clientSocketAddressSize = sizeof(clientSocketAddress);
    this->clientSocketFD = accept(this->serverSocketFD, &clientSocketAddress, &clientSocketAddressSize);
    if (this->clientSocketFD == -1)
        cout << "fail accept" << endl;
    else
        cout << "success accept" << endl;
}

void Program::describeSocketOption(void) {
    int optionValue;
    socklen_t optionLength = sizeof(optionValue);

    getsockopt(this->clientSocketFD, SOL_SOCKET, SO_SNDBUF, &optionValue, &optionLength);

    cout << "SO_SNDBUF: " << optionValue << ", size: " << optionLength << endl;
}

void Program::sendBigString(void) {
    std::string bigString = std::string(10, '1');
    ssize_t sendSize = send(this->clientSocketFD, bigString.c_str(), 10, 0);
    if (sendSize == -1)
        cout << "send error has occured" << endl;
    else
        cout << "sended big string" << endl;
}

void Program::closeServerSocket(void) {
    ::close(this->serverSocketFD);
    this->serverSocketFD = -1;

    cout << "closed server socket" << endl;
}

void Program::closeClientSocket(void) {
    ::close(this->clientSocketFD);
    this->clientSocketFD = -1;

    cout << "closed client socket" << endl;
}

void Program::close(void) {
    this->closeClientSocket();
    this->closeServerSocket();
}

void Program::recv(void) {
    char buf[1024 + 1];
    ssize_t recvSize = ::recv(this->clientSocketFD, buf, 1024, 0);
    if (recvSize == -1)
        throw "recv error has occured";
    else if (recvSize == 0)
        throw "no data";

    buf[recvSize] = '\0';

    cout << "received size: " << recvSize << endl;
    for (int index = 0; index < recvSize; index += 16) {
        print16Bytes(std::string(buf + index, buf + index + 16));
    }
}



// MARK: - program
void Program::mainLoop(void) {
    std::string line;

    while (true) {
        cout << endl << "Enable command list:" << endl;
        Program::describeMethodCommand();
        cout << "Enter command: ";

        lmi_getline(cin, line);

        Program::MethodMap::const_iterator iter = Program::methodMap.find(line);
        if (iter == Program::methodMap.end()) {
            cout << "not found: [" << line << "]" << endl;
            continue;
        }
        try {
            (this->*(iter->second))();
        } catch(const char* string) {
            cout << string << ": [" << line << "]" << endl;
        }
    }
}

void Program::describeMethodCommand(void) {
    for (MethodMap::const_iterator iter = Program::methodMap.begin(); iter != Program::methodMap.end(); ++iter)
        cout << "\t" << iter->first << endl;
}



// MARK: - static
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



// MARK: - main
int main(void) {
    Program program;

    program.mainLoop();

    return 0;
}
