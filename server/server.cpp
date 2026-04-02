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

void    Server::clientRemover(int fd){
    // FD'yi işletim sisteminde kapatmak (bağlantıyı tamamen kes)
    close(fd);

    // poll listesinden (vektörden temizlemek)
    for(size_t i = 0; i < _fds.size(); i++){
        if(_fds[i].fd == fd){
            _fds.erase(_fds.begin() + i);
            break;
        }
    }
    // kullanıcının bufferını temizleme
    _clientBuffers.erase(fd);
    std::cout << "FD " << fd << " left on the server and removed." << std::endl;
}


void    Server::handleClientData(int fd){
    char buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));

    // veriyi okuma işlemi
    ssize_t bytes = recv(fd, buffer, sizeof(buffer) - 1, 0);

    // hata veya kopma kontrolü

    if(bytes <= 0){
        if(bytes == 0)
            std::cout << "User shut the connection (FD: " << fd << ")" << std::endl;
        else
            std::cerr << "Error: Reading (FD: " << fd << ")" << std::endl;
        clientRemover(fd);
        return;
    }

    // veri başarıyla geldiyse map içindeki stringe ekleme
    _clientBuffers[fd] += buffer;

    // IRC satır sonu (\r\n) Kontrolü
    size_t pos;
    while((pos = _clientBuffers[fd].find("\r\n")) != std::string::npos){
        // komutu bastan basladıgı yere kadar al
        std::string command = _clientBuffers[fd].substr(0, pos);
        _clientBuffers[fd].erase(0, pos + 2);

        // test mesajı (silinebilir)
        std::cout << "Command [FD " << fd << "]: " << command << std::endl;
    }

}