#include "server.hpp"

Server::Server(int port, std::string password) : _port(port), _password(password), _serverFd(-1), _is_running(true){
    initCommands(); // komut haritası başladı öncesinde yoktu
}

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
    // artık gerçek poll yapısı
    while(Server::_signal == false){
        int poll_count = poll(&_fds[0], _fds.size(), -1);
        if(poll_count == -1 && Server::_signal == false)
            throw std::runtime_error("Poll error!");
        for(size_t i = 0; i < _fds.size(); i++){
            if(_fds[i].revents & POLLIN){
                if(_fds[i].fd == _serverFd)
                    acceptNewClient();
                else
                    handleClientData(_fds[i].fd);
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
    for(std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it){
        delete it->second; // içerideki new Client nesnelerini silip definitely lost'u engeller!
    }
    _clients.clear();
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
    // kullanıcının nesnesini ve bufferını map'ten temizleme
    if (_clients.count(fd)) {
        delete _clients[fd];
        _clients.erase(fd);
    }
    std::cout << "FD " << fd << " left on the server and removed." << std::endl;
}

void    Server::acceptNewClient(){
    struct sockaddr_in client_add;
    socklen_t client_len = sizeof(client_add);

    // yeni bağlantı kabulu
    int client_fd = accept(_serverFd, (struct sockaddr *)&client_add, &client_len);
    if(client_fd == -1){
        std::cerr << "Error: Cannot accept client!" << std::endl;
        return; 
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
    
    // yeni client'i map containerına ekliycez
    _clients[client_fd] = new Client(client_fd);
    std::cout << "New client connected! FD: " << client_fd << std::endl;
}

void Server::handleClientData(int fd){
    char buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));

    // veriyi okuma işlemi
    ssize_t bytes_received = recv(fd, buffer, sizeof(buffer) - 1, 0);

    // hata veya kopma kontrolü
    if(bytes_received <= 0){
        if(bytes_received == 0)
            std::cout << "User shut the connection (FD: " << fd << ")" << std::endl;
        else
            std::cerr << "Error: Reading (FD: " << fd << ")" << std::endl;
        clientRemover(fd);
        return;
    }

    //gelen veriyi o client'in kişisel bufferına eklememiz gerekiyor
    _clients[fd]->appendBuffer(buffer);

    // EVRENSEL BUFFER KONTROLÜ (Mac ve Ubuntu Uyumu)
    std::string &client_buffer = _clients[fd]->getBuffer();
    size_t pos;

    // \n görene kadar ara (Mac sadece \n yollar, Ubuntu \r\n yollar)
    while((pos = client_buffer.find('\n')) != std::string::npos){
        
        // \n'e kadar olan kısmı alıyoruz
        std::string command = client_buffer.substr(0, pos);

        // Eğer komutun sonunda \r kalmışsa onu da temizle (Ubuntu/Gerçek IRC uyumu)
        if (command.length() > 0 && command[command.length() - 1] == '\r') {
            command.erase(command.length() - 1);
        }

        // test mesajı (silinebilir)
        std::cout << "Command [FD " << fd << "]: " << command << std::endl;

        // komutu işlemek üzere parsere gönderiyoruz
        parseCommand(fd, command);

        // işlenen kısımı ve \n karakterini (pos + 1) bufferından siliyoruz
        _clients[fd]->eraseBuffer(pos + 1);
        client_buffer = _clients[fd]->getBuffer(); // referansı güncelliyoruz
    }
}

void Server::parseCommand(int fd, std::string command){
    std::vector<std::string> args;
    size_t pos = 0;
    std::string token; 

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

    if(_commands.find(cmd) != _commands.end()){
        // eğer tanımlıysa o komuta karşılık gelen fonksiyonu çalıştır
        (this->*_commands[cmd])(fd, args);
    }else
        std::cout << "Unknown command from FD " << fd << ": " << cmd << std::endl;
}

void    Server::cmdNick(int fd, std::vector<std::string> args){
    if(args.empty()){
        std::cout << "Error: NICK command needs a parameter!" << std::endl;
        return;
    }
    _clients[fd]->setNickSet(true);
    std::cout << "FD " << fd << " wants to change nick to: " << args[0] << std::endl;
    // !!!!hayri client sınıfını bitirince buraya _clients[fd]->setNickname(args[0]) yapıcam!!!!

    // kayıt kontrolü: user'dan önce nick girdi mi? ikisi de tamamsa kayıt biter
    if(_clients[fd]->isUserSet() && !_clients[fd]->isRegistered()){
        _clients[fd]->setRegistered(true);
        std::cout << "🎉 FD " << fd << " is fully registered to the server!" << std::endl;
        sendMessage(fd, ":ircserv 001 " + args[0] + " :Welcome to the ft_irc Network!");
    }
}

void    Server::cmdUser(int fd, std::vector<std::string> args){
    if(args.size() < 4){
        std::cout << "Error: USER command needs 4 parameters!" << std::endl;
        return;
    }
    // kullanıcının user komutunu girdiğini işaretliyoruz
    _clients[fd]->setUserSet(true);
    std::cout << "FD " << fd << " sent USER command." << std::endl;

    // kayıt kontrolü: kullanıcı user'dan önce nick girmiş mi? (ALT ÇİZGİ HATASI DÜZELTİLDİ)
    if(_clients[fd]->isNickSet() && !_clients[fd]->isRegistered()){
        _clients[fd]->setRegistered(true);
        std::cout << "🎉 FD " << fd << " is fully registered to the server!" << std::endl;
        
        sendMessage(fd, ":ircserv 001 * :Welcome to the ft_irc Network!");
    }
}

void    Server::initCommands(){
    // hangi komut geldiğinde hangi fonksiyonun çalışacağını burada ekliyoruz
    _commands["NICK"] = &Server::cmdNick;
    _commands["USER"] = &Server::cmdUser;

    // Hayri yeni komutlar ekleyince bende ekleme işlemine devam edeceğim
}

void    Server::sendMessage(int fd, std::string message){
    message += "\r\n";

    // send fonksitonu ile veriyi fd üzerinden netcat/ırc istemcisine göndericez
    if(send(fd, message.c_str(), message.size(), 0) == -1){
        std::cerr << "Error: Cannot send message to FD" << fd << std::endl;
    }
}