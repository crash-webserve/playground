#include <iostream>
#include <multimap>

namespace HTTP {

// class to store data of HTTP request message.
// Example:
//	// example of data in request_string
//	// "GET /index.html HTTP/1.1
//	//  Host: localhost
//	//
//	//  ";
//	Request request(request_string);
//	if (request.getMethod() == Request.GET)
//		server.runGetMessage(request);
class Request {
private:
	Method _method;
	std::string _request_target;
	char _major_version;
	char _minor_version;

	std::vector<std::string> _header_field_vector;

	std::string _body;

public:
	enum Method {
		GET,
		POST,
		DELETE,
	};

	explicit Request(std::string& string);

	Method getMethod() const;
};

// class to store data of HTTP response message.
// Example:
//	Response response = server.runMessage(request);
//	string response_string = response.convert_to_string();
//	send(client_socket, response_string, response_string.length(), MSG_DONTWAIT);
//	// example of data in response_string
//	// "HTTP/1.1 200 OK
//	//  Server: nginx/1.14.2
//	//  Content-Type: text/html
//	//  Content-Length: 612
//	//  Last-Modified: Tue, 04 Dec 2018 14:52:24 GMT
//	//  Connection: keep-alive
//	//  ETag: "5c0694a8-264"
//	//  Accept-Ranges: bytes
//	//
//	//  <!DOCTYPE html>
//  //  <html>
//  //  <head>
//  //  <title>Welcome to nginx!</title>
//  //  <style>
//  //      body {
//  //          width: 35em;
//  //          margin: 0 auto;
//  //          font-family: Tahoma, Verdana, Arial, sans-serif;
//  //      }
//  //  </style>
//  //  </head>
//  //  <body>
//  //  <h1>Welcome to nginx!</h1>
//  //  <p>If you see this page, the nginx web server is successfully installed and
//  //  working. Further configuration is required.</p>
//  //  
//  //  <p>For online documentation and support please refer to
//  //  <a href="http://nginx.org/">nginx.org</a>.<br/>
//  //  Commercial support is available at
//  //  <a href="http://nginx.com/">nginx.com</a>.</p>
//  //  
//  //  <p><em>Thank you for using nginx.</em></p>
//  //  </body>
//  //  </html>";
class Response {
private:
	char _major_version;
	char _minor_version;
	int _status_code;
	std::string _reason_phrase;

	std::vector<std::string> _header_field_vector;

	std::string _body;

public:
	std::string convertToString() const;
};

};
