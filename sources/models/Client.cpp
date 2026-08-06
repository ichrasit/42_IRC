#include "Client.hpp"

Client::Client(int fd) : _fd(fd), _buffer(""), _isNickSet(false), _isUserSet(false), 
                         _isRegistered(false), _isPassSet(false), 
                         _nickname(""), _username(""), _realname("") {
}

Client::~Client() {
}

int Client::getFd() const {
    return _fd;
}

void Client::appendBuffer(std::string str) {
    _buffer += str;
}

std::string& Client::getBuffer() {
    return _buffer;
}

void Client::clearBuffer() {
    _buffer.clear();
}

void Client::eraseBuffer(size_t pos) {
    _buffer.erase(0, pos);
}

bool Client::isNickSet() const {
    return _isNickSet;
}

void Client::setNickSet(bool status) {
    _isNickSet = status;
}

bool Client::isUserSet() const {
    return _isUserSet;
}

void Client::setUserSet(bool status) {
    _isUserSet = status;
}

bool Client::isRegistered() const {
    return _isRegistered;
}

void Client::setRegistered(bool status) {
    _isRegistered = status;
}

bool Client::isPassSet() const {
    return _isPassSet;
}

void Client::setPassSet(bool status) {
    _isPassSet = status;
}

std::string Client::getNickname() const {
    return _nickname.empty() ? "*" : _nickname;
}

void Client::setNickname(std::string nick) {
    _nickname = nick;
}

std::string Client::getUsername() const {
    return _username;
}

void Client::setUsername(std::string user) {
    _username = user;
}

std::string Client::getRealname() const {
    return _realname;
}

void Client::setRealname(std::string real) {
    _realname = real;
}