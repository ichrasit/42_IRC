#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include "server.hpp"


class Client{
    private:
        int _fd;
        std::string _buffer; // parçalı gelen mesajları burada biriktiricez!
    public:
        Client(int fd) : _fd(fd), _buffer("") {}
        ~Client(){}

        int getFd() const {return _fd; }

        // buffer işlemleri

        void appendBuffer(std::string str) {_buffer += str;}
        std::string getBuffer() const {return _buffer;}
        void clearBuffer() {_buffer.clear(); }
        void eraseBuffer(size_t pos) {_buffer.erase(0, pos);}
};


#endif
