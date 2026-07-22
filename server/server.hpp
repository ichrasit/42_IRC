#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <csignal> // Sinyal yonetimi icin (SIGINT vb.)
#include <cstring>
#include <unistd.h>
#include <sys/socket.h> // socket, bind ve listen fonksiyonlari
#include <netinet/in.h> // sockaddr_in yapisi
#include <fcntl.h>      // fcntl (non-blocking) icin
#include <poll.h>       // pollfd yapisi ve poll() fonksiyonu icin
#include <arpa/inet.h>  // inet_ntoa gibi donusumler icin
#include "client.hpp"   // Client sinif tanimi
#include <cctype>
#include "channel.hpp"

class Server {
    private:
        int         _port;
        std::string _password;
        int         _serverFd;
        
        // Ag yonetimi ve istemci takibi icin konteynerlar
        std::vector<struct pollfd> _fds;      // poll() fonksiyonunun izleyecegi soket listesi
        std::map<int, Client*>     _clients;  // FD'ye karsilik gelen Client nesneleri
        typedef void (Server::*CommandHandler)(int, std::vector<std::string>);
        
        std::map<std::string, CommandHandler> _commands;
        std::map<std::string, Channel*> _channels; // kanal ismine karsilik kanal nesnesi

    public:
        static bool _signal; // Sunucuyu kapatmak icin statik sinyal degiskeni
        
        Server(int port, std::string password);
        ~Server();

        // Sunucu temel islemleri
        void serverInitializer();
        void runner();
        void closerFds();

        // Baglanti ve Veri yonetimi
        void acceptNewClient();
        void handleClientData(int fd);
        void clientRemover(int fd);
        void sendMessage(int fd, std::string message);

        // IRC Protokol islemleri (Parser ve Komutlar)
        void parseCommand(int fd, std::string command);
        void cmdNick(int fd, std::vector<std::string> args);
        void cmdUser(int fd, std::vector<std::string> args);
        void initCommands();
        void cmdPass(int fd, std::vector<std::string> args);
        void cmdPing(int fd, std::vector<std::string> args);
        void cmdPrivmsg(int fd, std::vector<std::string> args);
        void cmdJoin(int fd, std::vector<std::string> args);
        void cmdPart(int fd, std::vector<std::string> args);
        void cmdQuit(int fd, std::vector<std::string> args);
        
        // Operatör ve Kanal Yetki Komutları
        void cmdKick(int fd, std::vector<std::string> args);   // KICK
        void cmdInvite(int fd, std::vector<std::string> args); // INVITE
        void cmdTopic(int fd, std::vector<std::string> args);  // TOPIC
        void cmdMode(int fd, std::vector<std::string> args);   // MODE
        
        void sendNumeric(int fd, std::string numeric, std::string message);
};

#endif