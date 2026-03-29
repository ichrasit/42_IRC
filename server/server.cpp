#include "server.hpp"

Server::Server(int port, std::string password) : _port(port), _password(password), _serverFd(-1), _is_running(true){}

Server::~Server(){
    closerFds();
}

void    Server::serverInitializer(){
    struct sockaddr_in add;
    struct pollfd serverPoll;

    // socket oluşturma
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if(_serverFd == -1)
        throw std::runtime_error("Socket is not created!");
    // portu yeniden kullanılabilir yapma (hızlı yeniden başlama)

    int en = 1;
    if(setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &en, sizeof(en)) == -1)
        throw std::runtime_error("Error : Setsockopt!");
    
    // non-blocking moduna geçiş : donma engelleme

    if(fcntl(_serverFd, F_SETFL, O_NONBLOCK) == -1)
        throw std::runtime_error("Error : fcntl!");
    
    // adres ve port yapılandırması

    add.sin_family = AF_INET; // IPV4 kullan
    add.sin_addr.s_addr = INADDR_ANY; // Gelen her ip'yi kabul et
    add.sin_port = htons(_port); // port'u internet byte sırasına çevir

    // sockete port bağlamak
    if(bind(_serverFd, (struct sockaddr *)&add, sizeof(add)) == -1)
        throw std::runtime_error("Error Bind! Port is probably full!");
    
    // bağlantıları beklemeye başkama

    if(listen(_serverFd, SOMAXCONN) == -1)
        throw std::runtime_error("Error : Listen");
    
    // gözcüye görev verme

    serverPoll.fd = _serverFd;
    serverPoll.events = POLLIN; // biri gelirse haber verir
    serverPoll.revents = 0;
    _fds.push_back(serverPoll);

    std::cout << "Server " << _port << " is listening" << std::endl;
}


void Server::runner(){
    // şimdilik sadece test yapısı
    while(Server::_signal == false){
        std::cout << "(poll) is waiting..." << std::endl;
        sleep(2);
    }

}

void    Server::closerFds(){
    //sunucu kapanırken açık olan tüm soketleri kapatma
    for(size_t i = 0; i < _fds.size(); i++){
        std::cout << "FD " << _fds[i].fd << " is closing." << std::endl;
        close(_fds[i].fd);
    }
    _fds.clear();
}