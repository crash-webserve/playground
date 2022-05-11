#ifndef CLASS_HPP_
#define CLASS_HPP_

#include <iostream>
#include <map>
#include <vector>

#include <arpa/inet.h>  // inet_addr()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h> // socket(), accept(), listen(), bind(), connect(), recv()
#include <sys/time.h>   // struct timespec
#include <fcntl.h>  // fcntl()
#include <sys/event.h>  // struct kevent, kevent()
#include <unistd.h>



typedef std::map<std::string, std::string> StringMap;

// class to store data of HTTP request message.
// Example:
//  // example of data in request_string
//  // "GET /index.html HTTP/1.1
//  //  Host: localhost
//  //
//  //  ";
//  Request request(request_string);
//  if (request.getMethod() == Request::METHOD_GET)
//      server.runMessage(request, response);
class Request {
public:
    enum Method {
        METHOD_GET,
        METHOD_POST,
        METHOD_DELETE,
    };

    void append(const char* str) { this->message += str; };
    void parse();

    Method getMethod() const;
    char getMajorVersion() const;
    char getMinorVersion() const;

    bool isReadyToService();

    void describe(std::ostream& out) const; // this is for debug usage

private:
    std::string message;

    Method _method;
    std::string _request_target;
    char _major_version;
    char _minor_version;

    StringMap _headerFieldMap;

    std::string _body;
};



struct Config {
public:
    Config(const char* configFilePath, char** invalidArgument);


public:
    std::string ipAddress;
    std::string portNumber;
};



class Sock {
public:
    enum Type {
        SERVER,
        CLIENT,
    };

    explicit Sock(Sock::Type type = Sock::CLIENT)
        : type(type) {};
    Sock(in_port_t portNumber, Sock::Type type);
    ~Sock();

    int getFD() const {return this->fd; };
    const std::string& getResponseMessage() const { return this->responseMessage; };
    void setResponseMessage(const std::string& responseMessage) { this->responseMessage = responseMessage; };

    bool isTypeServer() const { return this->type == Sock::SERVER; };
    bool isTypeClient() const { return this->type == Sock::CLIENT; };

    void accept(int kqueueFD, Sock& sock) const;
    ssize_t readRequest();

    bool isReadyToService() { return this->request.isReadyToService(); };
    void parseRequest() { this->request.parse(); };

private:
    Sock::Type type;

    struct sockaddr_in address;
    int fd;

    Request request;
    std::string responseMessage;
};



class SockMap {
public:
    typedef std::map<int, Sock*> Type;

    ~SockMap();

    Sock* operator[](const int& fd);
    void insert(const int& fd, Sock* sock);
    Type::size_type erase(const int& fd);

private:
    Type map;
};



class Server {
public:
    Server(const char* ipAddress, in_port_t portNumber, const std::string& serverName)
        : portNumber(portNumber), serverName(serverName) { inet_pton(AF_INET, ipAddress, &this->sinAddress); };

    in_port_t getPortNumber() const { return this->portNumber; };
    void setSock(Sock* sock);

    void provideService(Sock& sock, int kqueue) const;

private:
    struct in_addr sinAddress;
    in_port_t portNumber;
    std::string serverName;

    // 다른 많은 설정값

    Sock* sock;
};



typedef std::vector<Server*> ServerVectorType;
class ServerVector: public ServerVectorType {
public:
    ~ServerVector();
};



class ServerManager {
public:
    typedef std::map<std::string, std::string> StatusCodeMapType;

    ServerManager(Config& config);

    void eventLoop();

private:
    ServerVector serverVector;

    int kqueueFD;

    SockMap sockMap;

    StatusCodeMapType statusCodeMap;

    // in initializer
    void setUpServer(Config& config);
    void setUpSockMap();
    void initStatusCodeMap(const char* fileName);

    Server& getTargetServer(Sock& sock);
    
    // in loop
    void accept(const Sock& serverSock);
    void removeSock(int fd);
};




// class to store data of HTTP response message.
// Example:
//  server.runRequest(request, response);
//  std::string response_message = response.convertToString();
//  event_count = kevent();
//  for (int i = 0; i < event_count; ++i) {
//      if (event_array[i].ident == fd_to_send)
//          send(fd_to_send, response_message, response_message.length());
//  }
//  // example of data in response_message
//  // "HTTP/1.1 200 OK
//  //  Server: nginx/1.14.2
//  //  ...
//  //  Accept-Ranges: bytes
//  //
//  //  <!DOCTYPE html>
//  //  ...
//  //  </html>";
class Response {
public:
    Response();

    // setter
    void setMajorVersion(char new_value);
    void setMinorVersion(char new_value);
    void clearHeaderFieldMap();
    void insertHeaderFieldMap(const std::string& key, const std::string& value);
    void setBody(const std::string& new_value);

    void setStatus(const std::string& code);

    // initialize this->_status_reason_map
    // Call this function at the start of program
    static void initStatusCodeMap(const char* file_name);

    // generate response message
    std::string convertToString() const;

    void describe(std::ostream& out) const; // this is for debug usage

private:
    char _major_version;
    char _minor_version;
    char _status_code[4];
    std::string _reason_phrase;

    StringMap _headerFieldMap;

    std::string _body;

    static StringMap _status_reason_map;

    void setStatusCode(const char* new_value);
    void setReasonPhrase(const std::string& new_value);
};

#endif  // CLASS_HPP_
