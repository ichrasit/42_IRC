#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client {
    private:
        int         _fd;
        std::string _buffer;
        std::string _writeBuffer;
        bool        _markedForClose;
        
        bool        _isNickSet;
        bool        _isUserSet;
        bool        _isRegistered;
        bool        _isPassSet;
        
        std::string _nickname;
        std::string _username;
        std::string _realname;

    public:
        Client(int fd);
        ~Client();

        int getFd() const;

        void appendBuffer(std::string str);
        std::string& getBuffer();
        void clearBuffer();
        void eraseBuffer(size_t pos);

        void appendWrite(const std::string& str);
        std::string& getWriteBuffer();
        bool hasPendingWrite() const;

        bool isMarkedForClose() const;
        void markForClose();

        bool isNickSet() const;
        void setNickSet(bool status);
        
        bool isUserSet() const;
        void setUserSet(bool status);
        
        bool isRegistered() const;
        void setRegistered(bool status);
        
        bool isPassSet() const;
        void setPassSet(bool status);

        std::string getNickname() const;
        void setNickname(std::string nick);
        
        std::string getUsername() const;
        void setUsername(std::string user);
        
        std::string getRealname() const;
        void setRealname(std::string real);
};

#endif