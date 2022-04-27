#include <iostream>
#include <sstream>
#include <cassert>
#include <vector>
#include "message.hpp"

using std::cin;
using std::cout;
using std::endl;

static void describeStringVector(const std::vector<std::string>& vector);

const int NORMAL = 0;
const int ERROR = -1;

static void initMethod(HTTP::Request::Method& method, const std::string& token) {
	if (token == "GET")
		method = HTTP::Request::GET;
	else if (token == "POST")
		method = HTTP::Request::POST;
	else if (token == "DELETE")
		method = HTTP::Request::DELETE;
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

HTTP::Request::Request(const std::string& string) {
	std::stringstream ss(string);
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
		this->_header_field_vector.push_back(token);
		ss.get();
	}

	if (ss.eof())
		return;
	else
		ss.clear();

	this->_body = string.substr(ss.tellg());
}

HTTP::Request::Method HTTP::Request::getMethod() const {
	return this->_method;
}

char HTTP::Request::getMajorVersion() const {
	return this->_major_version;
}

char HTTP::Request::getMinorVersion() const {
	return this->_minor_version;
}

void HTTP::Request::describe() const {
	cout << "_method: [" << this->_method << "]" << endl;
	cout << "_request_target: [" << this->_request_target << "]" << endl;
	cout << "_major_version: [" << this->_major_version << "]" << endl;
	cout << "_minor_version: [" << this->_minor_version << "]" << endl;
	cout << endl;

	describeStringVector(this->_header_field_vector);
	cout << endl;

	cout << "_body: [" << this->_body << "]" << endl;
}

void HTTP::Response::setMajorVersion(char new_value) {
	this->_major_version = new_value;
}

void HTTP::Response::setMinorVersion(char new_value) {
	this->_minor_version = new_value;
}

void HTTP::Response::setStatusCode(int new_value) {
	this->_status_code = new_value;
}

void HTTP::Response::setReasonPhrase(const std::string& new_value) {
	this->_reason_phrase = new_value;
}

void HTTP::Response::clearHeaderFieldVector() {
	this->_header_field_vector.clear();
}

void HTTP::Response::appendHeaderFieldVector(const std::string& new_value) {
	this->_header_field_vector.push_back(new_value);
}

void HTTP::Response::setBody(const std::string& new_value) {
	this->_body = new_value;
}

std::string HTTP::Response::convertToString() const {
	std::string response_message;

	response_message += "HTTP/";
	response_message += this->_major_version;
	response_message += ".";
	response_message += this->_minor_version;
	response_message += " ";
	std::stringstream ss;
	ss << this->_status_code;
	response_message += ss.str();
	response_message += " ";
	response_message += this->_reason_phrase;
	response_message += "\r\n";
	for (std::vector<std::string>::const_iterator it = this->_header_field_vector.begin(); it != this->_header_field_vector.end(); ++it) {
		response_message += *it;
		response_message += "\r\n";
	}
	response_message += "\r\n";
	response_message += this->_body;

	return response_message;
}

void HTTP::Response::describe() const {
	cout << "_major_version: [" << this->_major_version << "]" << endl;
	cout << "_minor_version: [" << this->_minor_version << "]" << endl;
	std::stringstream ss;
	ss << this->_status_code;
	cout << "_status_code: [" << ss.str() << "]" << endl;
	cout << "_reason_phrase: [" << this->_reason_phrase << "]" << endl;
	cout << endl;

	describeStringVector(this->_header_field_vector);
	cout << endl;

	cout << "_body: [" << this->_body << "]" << endl;
}

void describeStringVector(const std::vector<std::string>& vector) {
	for (std::vector<std::string>::const_iterator it = vector.begin(); it != vector.end(); ++it) {
		cout << "header field: [";
		cout << *it;
		cout << "]" << endl;
	}
}

int Server::runGetRequest(const HTTP::Request& request, HTTP::Response& response) {
	(void)request;
	response.setMajorVersion(request.getMajorVersion());
	response.setMinorVersion(request.getMinorVersion());
	response.setStatusCode(200);
	response.setReasonPhrase("OK");

	response.clearHeaderFieldVector();
	response.appendHeaderFieldVector("Server: custom server");
	response.appendHeaderFieldVector("Date: Mon, 25 Apr 2022 05:38:34 GMT");
	response.appendHeaderFieldVector("Content-Type: text/html");
	response.appendHeaderFieldVector("Last-Modified: Tue, 04 Dec 2018 14:52:24 GMT");

	response.setBody("<!DOCFTYPE html><html></html>");

	return 0;
}

int Server::runPostRequest(const HTTP::Request& request, HTTP::Response& response) {
	(void)request;
	(void)response;

	return 0;
}

int Server::runDeleteRequest(const HTTP::Request& request, HTTP::Response& response) {
	(void)request;
	(void)response;

	return 0;
}

int Server::runRequest(const HTTP::Request& request, HTTP::Response& response) {
	return runGetRequest(request, response);
}
