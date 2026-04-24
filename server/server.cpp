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
        int poll_count = poll(&_fds[0], _fds.size(), -1);
        if(poll_count == -1 && Server::_signal == false)
            throw std::runtime_error("Poll error!");
        for(size_t i = 0; i < _fds.size(); i++){
            if(_fds[i].revents & POLLIN){
                if(_fds[i].fd == _serverFd)
                    acceptNewClient();
                else
                    std::cout << "A client sent a message! FD: " << _fds[i].fd << std::endl;
            }
        }
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

<<<<<<< HEAD
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

=======
void    Server::acceptNewClient(){
    struct sockaddr_in client_add;
    socklen_t client_len = sizeof(client_add);

    // yeni bağlantı kabulu
    int client_fd = accept(_serverFd, (struct sockaddr *)&client_add, &client_len);
    if(client_fd == -1){
        std::cerr << "Error: Cannot accept client!" << std::endl;
        return; // sunucu çökmesin diye throw yerine return kullandım
    }

    // yeni client'in soketini de non-blocking yapıyoruz
    if(fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1){
        std::cerr << "Error: fcntl failed for client!" << std::endl;
        close(client_fd);
        return;
    }

    // yeni client için yeni bir gözcü (pollfd) oluşturma

    struct pollfd client_poll;
    client_poll.fd = client_fd;
    client_poll.events = POLLIN;
    client_poll.revents = 0;

    // gözcüyü listeye ekliyoruz ki bir daha ki poll döngüsünde onu da dinleyebilelim
    _fds.push_back(client_poll);
    /* İleride Client sınıfını oluşturduğunda burada o nesneyi yaratıp map'e ekleyeceksin:
       _clients[client_fd] = new Client(client_fd, inet_ntoa(client_add.sin_addr));
    */
    
    // yeni client'i map containerına ekliycez
    _clients[client_fd] = new Client(client_fd);
    std::cout << "New client connected! FD: " << client_fd << std::endl;
}

void Server::handleClientData(int fd){
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytes_received = recv(fd, buffer, sizeof(buffer) - 1, 0);

    if(bytes_received <= 0){
        std::cout << "Client disconnected! FD: " << fd << std::endl;
        close(fd);
        clientRemover(fd);
    }else{
        //gelen veriyi o client'in kişisel bufferına eklememiz gerekiyor
        _clients[fd]->appendBuffer(buffer);

        // buffer'ın içinde "\r\n" var mı kontrolü varsa komut gelmiştir
        std::string &client_buffer = _clients[fd]->getBuffer();
        size_t pos;

        // içeride birden fazla komutda olabilir bu yüzden while
        while((pos = client_buffer.find("\r\n")) != std::string::npos){
            // \r\n ye kadar olan kısmı bölüyoruz bu tam irc komutu oluyor.
            std::string command = client_buffer.substr(0, pos);

            // komutu işlemek üzere parsere gönderiyoruz
            parseCommand(fd, command);

            // işlenen kısımı ve \r\n karakterlerini (pos + 2) bufferından siliyoruz
            _clients[fd]->eraseBuffer(pos + 2);
            client_buffer = _clients[fd]->getBuffer(); // referansı günceklliyoruz
        }
    }
}


void Server::parseCommand(int fd, std::string command){
    std::vector<std::string> args;
    size_t pos = 0;
    std::string tokene;

    // stringi boşluklara göre parçalama
    while((pos = command.find(' ')) != std::string::npos){
        token = command.substr(0, pos);
        if(!token.empty())
            args.push_back(token);
        command.erase(0, pos + 1);
    }
    if(!command.empty())
        args.push_back(command);

    // hiçbir şey gelmediyse çıkma işlemi
    if(args.empty()) return;
    
    std::string cmd = args[0]; // ilk kelime komutun kendisidir (nick user vb.)
    args.erase(args.begin()); // ilk kelimeyi listeden çıkar geriye sadece parametreler kalsın

    // yönlendirici
    if(cmd == "NICK")
        cmdNick(fd, args);
    else if(cmd == "USER")
        cmdUser(fd, args);
    else
        std::cout << "Unkown command from FD: " << fd << ": " << cmd << std::endl;
}

void    Server::cmdNick(int fd, std::vector<std::string> args){
    if(args.empty()){
        std::cout << "Error: NICK command needs a parameter!" << std::endl;
        return;
    }
    std::cout << "FD " << fd << " wants to change nıck to: " << args[0] << std::endl;
    // !!!!hayri client sınıfını bitirince buraya _clients[fd]->setNickname(args[0]) yapıcam!!!!

}

void    Server::cmdUser(int fd, std::vector<std::string> args){
    std::cout << "FD " << fd << " sent USER command." << std::endl; 
>>>>>>> 0436569 (ports are listening, adding a bit parse and prepareing client class for server site)
}