#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <map>

#include <arpa/inet.h>  // inet_addr()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h> // socket(), accept(), listen(), bind(), connect(), recv()
#include <sys/time.h>   // struct timespec
#include <fcntl.h>  // fcntl()
#include <sys/event.h>  // struct kevent, kevent()
#include <unistd.h>

#include "class.hpp"

using std::cin;
using std::cout;
using std::endl;

static void describeStringMap(std::ostream& out, const StringMap& map);
static void initMethod(Request::Method& method, const std::string& token);
static int initVersion(char& major_version, char& minor_version, const std::string& token);
static void addReadEvent(int sock, int kqueueFD);
static void addEvent(int sock, int kqueueFD, int16_t filter, void* udata);

const int NORMAL = 0;
const int ERROR = -1;

const std::size_t DEFAULT_EVENT_ARRAY_SIZE = 32;
const std::size_t BUF_SIZE = 512;



Config::Config(const char* configFilePath, char** invalidArgument) {
    (void)configFilePath;

    this->ipAddress = "127.0.0.1";
    this->portNumber = invalidArgument[1];
}

Sock::Sock(in_port_t portNumber, Sock::Type type): type(type) {
    this->fd = socket(PF_INET, SOCK_STREAM, 0);

    fcntl(this->fd, F_SETFL, O_NONBLOCK);

    memset(&this->address, 0, sizeof(this->address));
    this->address.sin_family = AF_INET;
    this->address.sin_addr.s_addr = htonl(INADDR_ANY);
    this->address.sin_port = htons(portNumber);

    if (bind(this->fd, (struct sockaddr*) &this->address, sizeof(this->address)) == -1)
        cout << "fail bind()" << endl;
    else
        cout << "success bind()" << endl;
}

Sock::~Sock() {
    close(this->fd);
}

void Sock::accept(int kqueueFD, Sock& sock) const {
    socklen_t sizeOfClientSockAddress = sizeof(sock.address);
    int clientSock = ::accept(this->fd, (struct sockaddr*)&sock.address, &sizeOfClientSockAddress);
    addReadEvent(clientSock, kqueueFD);
}

ssize_t Sock::readRequest() {
    char buf[BUF_SIZE];
    const ssize_t countOfReceivedBytes = recv(this->fd, buf, BUF_SIZE - 1, 0);
    buf[countOfReceivedBytes] = '\0';
    this->request.append(buf);

    return countOfReceivedBytes;
}

SockMap::~SockMap() {
    for (SockMap::Type::iterator iter = this->map.begin(); iter != this->map.end(); ++iter) {
        delete iter->second;
    }
}

Sock* SockMap::operator[](const int& fd) {
    const SockMap::Type::iterator iter = this->map.find(fd);
    if (iter == this->map.end())
        return NULL;

    return iter->second;
}

void SockMap::insert(const int& fd, Sock* sock) {
    const SockMap::Type::iterator iter = this->map.find(fd);

    if (iter != this->map.end())
        delete iter->second;

    this->map[fd] = sock;
}

SockMap::Type::size_type SockMap::erase(const int& fd) {
    const SockMap::Type::iterator iter = this->map.find(fd);
    if (iter != this->map.end())
        delete iter->second;

    return this->map.erase(fd);
}

void Server::setSock(Sock* sock) {
    this->sock = sock;
}

void Server::provideService(Sock& sock, int kqueueFD) const {
    sock.parseRequest();

    Response response;

    response.setMajorVersion('1');
    response.setMinorVersion('1');
    response.setStatus("200");

    response.clearHeaderFieldMap();
    response.insertHeaderFieldMap("Server", "custom server");
    response.insertHeaderFieldMap("Date", "Mon, 25 Apr 2022 05:38:34 GMT");
    response.insertHeaderFieldMap("Content-Type", "image/png");
    response.insertHeaderFieldMap("Last-Modified", "Tue, 04 Dec 2018 14:52:24 GMT");

    std::string str = std::string(0x1 << 10, 'a');
    response.setBody(str);

    sock.setResponseMessage(response.convertToString());

    addEvent(sock.getFD(), kqueueFD, EVFILT_WRITE, (void*)&sock.getResponseMessage());
}

ServerVector::~ServerVector() {
    for (ServerVectorType::iterator iter = this->begin(); iter != this->end(); ++iter) {
        delete *iter;
    }
}

ServerManager::ServerManager(Config& config) {
    this->setUpServer(config);
    this->kqueueFD = kqueue();
    this->setUpSockMap();
    this->initStatusCodeMap(NULL);
}

void ServerManager::eventLoop() {
    int eventCount;
    struct kevent eventArray[DEFAULT_EVENT_ARRAY_SIZE];
    std::string responseMessageArray[DEFAULT_EVENT_ARRAY_SIZE];
    struct timespec timeout = { 0, 0 };
    fcntl(0, F_SETFL, O_NONBLOCK);

    while (cin.get() != 'q') {
        cin.clear();
        eventCount = kevent(kqueueFD, NULL, 0, eventArray, DEFAULT_EVENT_ARRAY_SIZE, &timeout);
        if (eventCount != 0)
        for (int i = 0; i < eventCount; ++i) {
            const struct kevent& event = eventArray[i];
            const int fd = event.ident;
            Sock& sock = *this->sockMap[fd];

            if (event.filter == EVFILT_READ) {
                if (sock.isTypeServer())
                    this->accept(sock);
                else if (sock.isTypeClient()) {
                    if (sock.readRequest() != 0) {
                        if (sock.isReadyToService()) {
                            const Server& targetServer = this->getTargetServer(sock);
                            targetServer.provideService(sock, this->kqueueFD);
                        }
                    }
                    else
                        this->removeSock(fd);
                }
            }
            else if (event.filter == EVFILT_WRITE) {
                const std::string& responseMessage = (const std::string&)event.udata;
                send(fd, responseMessage.c_str(), responseMessage.length(), 0);
            }
        }

        usleep(100000);
    }
}

void ServerManager::setUpServer(Config& config) {
    Server* const newServer = new Server(config.ipAddress.c_str(), atoi(config.portNumber.c_str()), "localhost");
    this->serverVector.push_back(newServer);
}

void ServerManager::setUpSockMap() {
    std::map<in_port_t, int> portFDMap;

    for (ServerVectorType::iterator iter = this->serverVector.begin(); iter != this->serverVector.end(); ++iter) {
        Server& server = **iter;
        const in_port_t portNumber = server.getPortNumber();

        if (portFDMap.count(portNumber) == 0) {
            Sock* const newSock = new Sock(portNumber, Sock::SERVER);
            const int fd = newSock->getFD();

            this->sockMap.insert(fd, newSock);
            portFDMap[portNumber] = fd;

            listen(newSock->getFD(), 5);
            addReadEvent(newSock->getFD(), this->kqueueFD);
        }

        const int fd = portFDMap[portNumber];
        Sock* const sock = this->sockMap[fd];
        server.setSock(sock);
    }
}

void ServerManager::initStatusCodeMap(const char* fileName) {
    std::string fileNameString;
    if (fileName != NULL)
        fileNameString = fileName;
    else
        fileNameString = "status_code.txt";

    std::ifstream fin(fileNameString.c_str());
    if (!fin.is_open())
        return;

    std::string code;
    std::string reason;
    while (fin >> code && fin.get() && getline(fin, reason)) {
        this->statusCodeMap[code] = reason;
    }
}

void ServerManager::accept(const Sock& serverSock) {
    Sock* newSock = new Sock();
    serverSock.accept(this->kqueueFD, *newSock);
    this->sockMap.insert(newSock->getFD(), newSock);
    addReadEvent(newSock->getFD(), this->kqueueFD);
}

void ServerManager::removeSock(int fd) {
    this->sockMap.erase(fd);
}

// TODO implement real behavior
Server& ServerManager::getTargetServer(Sock& sock) {
    (void)sock;
    return *this->serverVector[0];
}

void Request::parse() {
    std::stringstream ss(this->message);
    std::string token;

    // TODO extraction error if not, abort might occur
    ss >> token;
    initMethod(this->_method, token);

    ss >> token;
    this->_request_target = token;

    ss >> token;
    if (initVersion(this->_major_version, this->_minor_version, token) == ERROR)
        cout << "error occured" << endl;

    char ch;
    while (isspace(ch = ss.get()));
    ss.putback(ch);
    while (std::getline(ss, token, (char)0x0d)) {
        if (token.length() == 0) {
            ss.get();
            break;
        }
        std::string::size_type colon_position = token.find(':');
        const std::string key = token.substr(0, colon_position);
        const std::string value = token.substr(colon_position + 2);
        this->_headerFieldMap[key] = value;
        ss.get();
    }

    if (ss.eof())
        return;
    else
        ss.clear();

    this->_body = this->message.substr(ss.tellg());
}

Request::Method Request::getMethod() const {
    return this->_method;
}

char Request::getMajorVersion() const {
    return this->_major_version;
}

char Request::getMinorVersion() const {
    return this->_minor_version;
}

// TODO implement real behavior
bool Request::isReadyToService() {
    return true;
}

void Request::describe(std::ostream& out) const {
    out << "_method: [" << this->_method << "]" << endl;
    out << "_request_target: [" << this->_request_target << "]" << endl;
    out << "_major_version: [" << this->_major_version << "]" << endl;
    out << "_minor_version: [" << this->_minor_version << "]" << endl;
    out << endl;

    out << "_headerFieldMap:" << endl;
    describeStringMap(out, this->_headerFieldMap);
    out << endl;

    out << "_body: [" << this->_body << "]" << endl;
}

StringMap Response::_status_reason_map = StringMap();

Response::Response() {
    this->_status_code[3] = '\0';
}

void Response::setMajorVersion(char new_value) {
    this->_major_version = new_value;
}

void Response::setMinorVersion(char new_value) {
    this->_minor_version = new_value;
}

void Response::setStatusCode(const char* new_value) {
    memcpy(this->_status_code, new_value, 3);
}

void Response::setReasonPhrase(const std::string& new_value) {
    this->_reason_phrase = new_value;
}

void Response::clearHeaderFieldMap() {
    this->_headerFieldMap.clear();
}

void Response::insertHeaderFieldMap(const std::string& key, const std::string& value) {
    this->_headerFieldMap[key] = value;
}

void Response::setBody(const std::string& new_value) {
    this->_body = new_value;
}

void Response::setStatus(const std::string& code) {
    this->setStatusCode(code.c_str());
    this->setReasonPhrase(Response::_status_reason_map[code].c_str());
}

std::string Response::convertToString() const {
    std::string response_message;

    response_message += "HTTP/";
    response_message += this->_major_version;
    response_message += ".";
    response_message += this->_minor_version;
    response_message += " ";
    response_message += this->_status_code;
    response_message += " ";
    response_message += this->_reason_phrase;
    response_message += "\r\n";
    for (StringMap::const_iterator it = this->_headerFieldMap.begin(); it != this->_headerFieldMap.end(); ++it) {
        response_message += it->first;
        response_message += ": ";
        response_message += it->second;
        response_message += "\r\n";
    }
    response_message += "\r\n";
    response_message += this->_body;

    return response_message;
}

void Response::describe(std::ostream& out) const {
    out << "_status_reason_map:" << endl;
    describeStringMap(out, Response::_status_reason_map);
    out << endl;

    out << "_major_version: [" << this->_major_version << "]" << endl;
    out << "_minor_version: [" << this->_minor_version << "]" << endl;
    out << "_status_code: [" << this->_status_code << "]" << endl;
    out << "_reason_phrase: [" << this->_reason_phrase << "]" << endl;
    out << endl;

    out << "_headerFieldMap:" << endl;
    describeStringMap(out, this->_headerFieldMap);
    out << endl;

//  out << "_body: [" << this->_body << "]" << endl;
}

void describeStringMap(std::ostream& out, const StringMap& map) {
    for (StringMap::const_iterator it = map.begin(); it != map.end(); ++it) {
        out << "pair: { key: [";
        out << it->first;
        out << "], value: [";
        out << it->second;
        out << "] }" << endl;
    }
}

static void initMethod(Request::Method& method, const std::string& token) {
    if (token == "GET")
        method = Request::METHOD_GET;
    else if (token == "POST")
        method = Request::METHOD_POST;
    else if (token == "DELETE")
        method = Request::METHOD_DELETE;
    else
        assert(false);
}

static int initVersion(char& major_version, char& minor_version, const std::string& token) {
    if (token.length() != 8)
        return ERROR;

    if (token.substr(0, 5) != "HTTP/")
        return ERROR;

    if (token[6] != '.')
        return ERROR;

    if (!isdigit(token[5]) || !isdigit(token[7]))
        return ERROR;

    major_version = token[5];
    minor_version = token[7];

    return NORMAL;
}

// event
static void addReadEvent(int sock, int kqueueFD) {
    addEvent(sock, kqueueFD, EVFILT_READ, NULL);
}

static void addEvent(int sock, int kqueueFD, int16_t filter, void* udata) {
    struct kevent event;
    EV_SET(&event, sock, filter, EV_ADD, 0, 0, udata);
    kevent(kqueueFD, &event, 1, NULL, 0, NULL);
}
