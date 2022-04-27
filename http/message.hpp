#ifndef MESSAGE_HPP_
#define MESSAGE_HPP_

#include <iostream>
#include <map>

namespace HTTP {

// class to store data of HTTP request message.
// Example:
//	// example of data in request_string
//	// "GET /index.html HTTP/1.1
//	//  Host: localhost
//	//
//	//  ";
//	HTTP::Request request(request_string);
//	if (request.getMethod() == HTTP::Request::GET)
//		server.runMessage(request, response);
class Request {
public:
	enum Method {
		GET,
		POST,
		DELETE,
	};

private:
	Method _method;
	std::string _request_target;
	char _major_version;
	char _minor_version;

	std::map<std::string, std::string> _header_field_map;

	std::string _body;

public:
	explicit Request(const std::string& string);

	Method getMethod() const;
	char getMajorVersion() const;
	char getMinorVersion() const;

	void describe() const;	// this is for debug usage
};

// class to store data of HTTP response message.
// Example:
//  server.runRequest(request, response);
//  std::string response_message = response.convertToString();
//  event_count = kevent();
//  for (int i = 0; i < event_count; ++i) {
//  	if (event_array[i].ident == fd_to_send)
//  		send(fd_to_send, response_message, response_message.length());
//  }
//	// example of data in response_message
//	// "HTTP/1.1 200 OK
//	//  Server: nginx/1.14.2
//	//	...
//	//  Accept-Ranges: bytes
//	//
//	//  <!DOCTYPE html>
//  //  ...
//  //  </html>";
class Response {
private:
	char _major_version;
	char _minor_version;
	char _status_code[4];
	std::string _reason_phrase;

	std::map<std::string, std::string> _header_field_map;

	std::string _body;

public:
	Response();
	// setter
	void setMajorVersion(char new_value);
	void setMinorVersion(char new_value);
	void setStatusCode(const char* new_value);
	void setReasonPhrase(const std::string& new_value);
	void clearHeaderFieldMap();
	void appendHeaderFieldMap(const std::string& key, const std::string& value);
	void setBody(const std::string& new_value);

	// generate response message
	std::string convertToString() const;

	void describe() const;	// this is for debug usage
};

};	// namespace HTTP

// class for to interact with messages
// Example:
//  server.runRequest(request, response);
//  std::string response_message = response.convertToString();
//  event_count = kevent();
//  for (int i = 0; i < event_count; ++i) {
//  	if (event_array[i].ident == fd_to_send)
//  		send(fd_to_send, response_message, response_message.length());
//  }
class Server {
private:
	int runGetRequest(const HTTP::Request& request, HTTP::Response& response);
	int runPostRequest(const HTTP::Request& request, HTTP::Response& response);
	int runDeleteRequest(const HTTP::Request& request, HTTP::Response& response);
	// other members

public:
	int runRequest(const HTTP::Request& request, HTTP::Response& response);
	// other members
};

#endif	// MESSAGE_HPP_
