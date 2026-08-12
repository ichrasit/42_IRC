#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <ctime>

class Client{
    private:
        int         _fd;
        std::string _buffer; // parca gelen mesajlari burada biriktiricez!
        bool        _isNickSet;
        bool        _isUserSet;
        bool        _isRegistered;
        bool        _isPassSet;

        // yeni eklenen kimlik bilgileri
        std::string _nickname;
        std::string _username;
        std::string _realname;

        // zombi kontrol degiskenleri
        time_t      _lastActivity;
        bool        _pingSent;

    public:
        Client(int fd) : _fd(fd), _buffer(""), _isNickSet(false), _isUserSet(false), _isRegistered(false), _isPassSet(false), _nickname(""), _username(""), _realname(""), _lastActivity(time(NULL)), _pingSent(false) {}
        ~Client(){}

        int getFd() const {return _fd; }

        // buffer islemleri
        void appendBuffer(std::string str) {_buffer += str;}
        std::string& getBuffer() { return _buffer; }       
        void clearBuffer() {_buffer.clear(); }
        void eraseBuffer(size_t pos) {_buffer.erase(0, pos);}

        // getter ve setter fonksiyonlari
        bool isNickSet() const { return _isNickSet; }
        void setNickSet(bool status) { _isNickSet = status; }
        bool isUserSet() const { return _isUserSet; }
        void setUserSet(bool status) { _isUserSet = status; }
        bool isRegistered() const { return _isRegistered; }
        void setRegistered(bool status) { _isRegistered = status; }
        bool isPassSet() const { return _isPassSet; }
        void setPassSet(bool status) { _isPassSet = status; }

        // nickname username ve realname getter setterlari
        std::string getNickname() const { return _nickname.empty() ? "*" : _nickname; }
        void setNickname(std::string nick) { _nickname = nick; }
        std::string getUsername() const { return _username; }
        void setUsername(std::string user) { _username = user; }
        std::string getRealname() const { return _realname; }
        void setRealname(std::string real) { _realname = real; }

        // zombi kontrol fonksiyonlari
        time_t getLastActivity() const { return _lastActivity; }
        bool isPingSent() const { return _pingSent; }
        void setPingSent(bool status) { _pingSent = status; }
        void updateLastActivity() {
            _lastActivity = time(NULL);
            _pingSent = false;
        }
};

#endif