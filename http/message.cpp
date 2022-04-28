#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <map>
#include "message.hpp"

using std::cin;
using std::cout;
using std::endl;

static void describeStringMap(const HTTP::StringMap& map);

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
		std::string::size_type colon_position = token.find(':');
		const std::string key = token.substr(0, colon_position);
		const std::string value = token.substr(colon_position + 2);
		this->_header_field_map[key] = value;
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

	describeStringMap(this->_header_field_map);
	cout << endl;

	cout << "_body: [" << this->_body << "]" << endl;
}

HTTP::StringMap HTTP::Response::_status_reason_map = HTTP::StringMap();

HTTP::Response::Response() {
	this->_status_code[3] = '\0';
}

void HTTP::Response::setMajorVersion(char new_value) {
	this->_major_version = new_value;
}

void HTTP::Response::setMinorVersion(char new_value) {
	this->_minor_version = new_value;
}

void HTTP::Response::setStatusCode(const char* new_value) {
	memcpy(this->_status_code, new_value, 3);
}

void HTTP::Response::setReasonPhrase(const std::string& new_value) {
	this->_reason_phrase = new_value;
}

void HTTP::Response::clearHeaderFieldMap() {
	this->_header_field_map.clear();
}

void HTTP::Response::insertHeaderFieldMap(const std::string& key, const std::string& value) {
	this->_header_field_map[key] = value;
}

void HTTP::Response::setBody(const std::string& new_value) {
	this->_body = new_value;
}

void HTTP::Response::setStatus(const std::string& code) {
	this->setStatusCode(code.c_str());
	this->setReasonPhrase(HTTP::Response::_status_reason_map[code].c_str());
}

void HTTP::Response::initStatusCodeMap(const char* file_name) {
	std::string file_name_string;
	if (file_name != NULL)
		file_name_string = file_name;
	else
		file_name_string = "status_code.txt";

	std::ifstream fin(file_name_string.c_str());
	if (!fin.is_open())
		return;

	std::string code;
	std::string reason;
	while (fin >> code && fin.get() && getline(fin, reason)) {
		HTTP::Response::_status_reason_map[code] = reason;
	}

	describeStringMap(HTTP::Response::_status_reason_map);
}

std::string HTTP::Response::convertToString() const {
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
	for (HTTP::StringMap::const_iterator it = this->_header_field_map.begin(); it != this->_header_field_map.end(); ++it) {
		response_message += it->first;
		response_message += ": ";
		response_message += it->second;
		response_message += "\r\n";
	}
	response_message += "\r\n";
	response_message += this->_body;

	return response_message;
}

void HTTP::Response::describe() const {
	cout << "_major_version: [" << this->_major_version << "]" << endl;
	cout << "_minor_version: [" << this->_minor_version << "]" << endl;
	cout << "_status_code: [" << this->_status_code << "]" << endl;
	cout << "_reason_phrase: [" << this->_reason_phrase << "]" << endl;
	cout << endl;

	describeStringMap(this->_header_field_map);
	cout << endl;

	cout << "_body: [" << this->_body << "]" << endl;
}

void describeStringMap(const HTTP::StringMap& map) {
	for (HTTP::StringMap::const_iterator it = map.begin(); it != map.end(); ++it) {
		cout << "header field: { key: [";
		cout << it->first;
		cout << "], value: [";
		cout << it->second;
		cout << "] }" << endl;
	}
}

int Worker::runGetRequest(const HTTP::Request& request, HTTP::Response& response) {
	(void)request;
	response.setMajorVersion(request.getMajorVersion());
	response.setMinorVersion(request.getMinorVersion());
	response.setStatus("200");

	response.clearHeaderFieldMap();
	response.insertHeaderFieldMap("Server", "custom server");
	response.insertHeaderFieldMap("Date", "Mon, 25 Apr 2022 05:38:34 GMT");
	response.insertHeaderFieldMap("Content-Type", "text/html");
	response.insertHeaderFieldMap("Last-Modified", "Tue, 04 Dec 2018 14:52:24 GMT");

	response.setBody("<!DOCFTYPE html><html></html>");

	return 0;
}

int Worker::runPostRequest(const HTTP::Request& request, HTTP::Response& response) {
	(void)request;
	(void)response;

	return 0;
}

int Worker::runDeleteRequest(const HTTP::Request& request, HTTP::Response& response) {
	(void)request;
	(void)response;

	return 0;
}

int Worker::runRequest(const HTTP::Request& request, HTTP::Response& response) {
	return runGetRequest(request, response);
}
