#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client{
    private:
        int _fd;
        std::string _buffer; // parçalı gelen mesajları burada biriktiricez!

        bool _isNickSet;
        bool _isUserSet;
        bool _isRegistered;
        bool _isPassSet = false;
    public:
        Client(int fd) : _fd(fd), _buffer(""), _isNickSet(false), _isUserSet(false), _isRegistered(false) {}
        ~Client(){}

        int getFd() const {return _fd; }

        // buffer işlemleri
        void appendBuffer(std::string str) {_buffer += str;}
        std::string& getBuffer() { return _buffer; }       
        void clearBuffer() {_buffer.clear(); }
        void eraseBuffer(size_t pos) {_buffer.erase(0, pos);}

        // getter ve setter fonksiyonları!
        bool isNickSet() const { return _isNickSet; }
        void setNickSet(bool status) { _isNickSet = status; }

        bool isUserSet() const { return _isUserSet; }
        void setUserSet(bool status) { _isUserSet = status; }

        bool isRegistered() const { return _isRegistered; }
        void setRegistered(bool status) { _isRegistered = status; }
        bool isPassSet() {return _isPassSet;}
        void setPassSet(bool status) {_isPassSet = status;}
};

#endif