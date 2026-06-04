#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <csignal> // Sinyal yönetimi için (SIGINT vb.)
#include <cstring>
#include <unistd.h>
#include <sys/socket.h> // socket, bind ve listen fonksiyonları için
#include <netinet/in.h> // sockaddr_in yapısı için
#include <fcntl.h>      // fcntl (non-blocking) için
#include <poll.h>       // pollfd yapısı ve poll() fonksiyonu için
#include <arpa/inet.h>  // inet_ntoa gibi dönüşümler için
#include "client.hpp"   // Client sınıfı tanımı için
#include <cctype>
#include "channel.hpp"
class Server {
    private:
        int         _port;
        std::string _password;
        int         _serverFd;
        bool        _is_running;
        
        // Ağ yönetimi ve istemci takibi için konteynerlar
        std::vector<struct pollfd> _fds;      // poll() fonksiyonunun izleyeceği soket listesi
        std::map<int, Client*>     _clients;  // FD'ye karşılık gelen Client nesneleri
        typedef void (Server::*CommandHandler)(int, std::vector<std::string>);
        
        std::map<std::string, CommandHandler> _commands;
        std::map<std::string, Channel*> _channels; // kanal ismine karşılık kanal nesnesi
    public:
        static bool _signal; // Sunucuyu kapatmak için statik sinyal değişkeni
        
        Server(int port, std::string password);
        ~Server();

        // Sunucu temel işlemleri
        void serverInitializer();
        void runner();
        void closerFds();

        // Bağlantı ve Veri yönetimi
        void acceptNewClient();
        void handleClientData(int fd);
        void clientRemover(int fd);

        void sendMessage(int fd, std::string message);
        // IRC Protokol işlemleri (Parser ve Komutlar)
        void parseCommand(int fd, std::string command);
        void cmdNick(int fd, std::vector<std::string> args);
        void cmdUser(int fd, std::vector<std::string> args);
        void initCommands();
}; 


#endif