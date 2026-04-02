#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <csignal> // ubuntu sinyal için
#include <cstring>
#include <unistd.h>
#include <sys/socket.h> // socket, bind ve listen için gerekli olan kütüphanemiz
#include <netinet/in.h> // sockaddr_in yapısı için gerekli olan kütüphanemiz
#include <fcntl.h>
#include <poll.h> // pollfd yapısı ve poll() fonskiyonumuz için
#include <arpa/inet.h> // inet_itoa iiçin


// client sınıfı ilerde tanımlanacak
class Client;

class Server{
    private:
        int _port;
        std::string _password;
        int _serverFd;
        bool    _is_running;
        std::map<int, std::string> _clientBuffers; // FD ve o FD'ye ait yarım mesajları tutan buffer
        
        // Ağ yönetimi için gerekli olan konteynerlar
        std::vector<struct pollfd> _fds;
        std::map<int, Client*>  _clients;
        
    public:
        static bool _signal;
        Server(int port, std::string password);
        ~Server();

        void serverInitializer();
        void runner();
        void closerFds();

        void acceptNewClient();
        void handleClientData(int fd);

        void clientRemover(int fd);
}; 
#endif