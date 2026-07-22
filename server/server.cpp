#include "server.hpp"

Server::Server(int port, std::string password) : _port(port), _password(password), _serverFd(-1){
    initCommands(); // komut haritasi
    _channels["#42test"] = new Channel("#42test");
}
Server::~Server(){
    closerFds();
}

void    Server::serverInitializer(){
    struct sockaddr_in add;
    struct pollfd serverPoll;

    // socket olusturma
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if(_serverFd == -1)
        throw std::runtime_error("Socket is not created!");
    
    // portu yeniden kullanilabilir yapma (hizli yeniden baslatma)
    int en = 1;
    if(setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &en, sizeof(en)) == -1)
        throw std::runtime_error("Error : Setsockopt!");
    
    // non-blocking moduna gecis : donma engelleme
    if(fcntl(_serverFd, F_SETFL, O_NONBLOCK) == -1)
        throw std::runtime_error("Error : fcntl!");
    
    // adres ve port yapilandirmasi
    add.sin_family = AF_INET; // IPV4 kullan
    add.sin_addr.s_addr = INADDR_ANY; // Gelen her ip'yi kabul et
    add.sin_port = htons(_port); // port'u internet byte sırasına cevir

    // sockete port baglamak
    if(bind(_serverFd, (struct sockaddr *)&add, sizeof(add)) == -1)
        throw std::runtime_error("Error Bind! Port is probably full!");
    
    // baglanti beklemeye baslama
    if(listen(_serverFd, SOMAXCONN) == -1)
        throw std::runtime_error("Error : Listen");
    
    // gozcuye gorev verme
    serverPoll.fd = _serverFd;
    serverPoll.events = POLLIN; // biri gelirse haber verir
    serverPoll.revents = 0;
    _fds.push_back(serverPoll);

    std::cout << "Server " << _port << " is listening" << std::endl;
}

void Server::runner(){
    // artik gercek poll yapisi
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

void Server::closerFds(){
    // sunucu kapanirken acik olan tum soketleri kapatma
    for(size_t i = 0; i < _fds.size(); i++){
        std::cout << "FD " << _fds[i].fd << " is closing." << std::endl;
        close(_fds[i].fd);
    }
    // Client'lari temizleme
    for(std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it){
        delete it->second; 
    }
    _clients.clear();
    _fds.clear();

    // Kanallari temizleme
    for(std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it){
        delete it->second; 
    }
    _channels.clear();

    std::cout << "All channels are deleted and memory is freed." << std::endl;
}

void    Server::clientRemover(int fd){
    // FD'yi isletim sisteminde kapatmak (baglanti tamamen kesilir)
    close(fd);

    // poll listesinden (vektorden temizlemek)
    for(size_t i = 0; i < _fds.size(); i++){
        if(_fds[i].fd == fd){
            _fds.erase(_fds.begin() + i);
            break;
        }
    }

    // kullanici nesnesini ve bufferi map'ten temizleme
    if (_clients.count(fd)) {
        delete _clients[fd];
        _clients.erase(fd);
    }
    std::cout << "FD " << fd << " left on the server and removed." << std::endl;
}

void    Server::acceptNewClient(){
    struct sockaddr_in client_add;
    socklen_t client_len = sizeof(client_add);

    // yeni baglanti kabulu
    int client_fd = accept(_serverFd, (struct sockaddr *)&client_add, &client_len);
    if(client_fd == -1){
        std::cerr << "Error: Cannot accept client!" << std::endl;
        return; 
    }

    // yeni client'in soketini de non-blocking yapiyoruz
    if(fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1){
        std::cerr << "Error: fcntl failed for client!" << std::endl;
        close(client_fd);
        return;
    }

    // yeni client icin yeni bir gozcu (pollfd) olusturma
    struct pollfd client_poll;
    client_poll.fd = client_fd;
    client_poll.events = POLLIN;
    client_poll.revents = 0;

    // gozcuyu listeye ekliyoruz ki bir daha ki poll dongusunde onu da dinleyebilelim
    _fds.push_back(client_poll);
    
    // yeni client'i map containerina ekliycez
    _clients[client_fd] = new Client(client_fd);

    std::cout << "New client connected! FD: " << client_fd << std::endl;
}

void Server::handleClientData(int fd){
    char buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));

    // veriyi okuma islemi
    ssize_t bytes_received = recv(fd, buffer, sizeof(buffer) - 1, 0);

    // hata veya kopma kontrolu
    if(bytes_received <= 0){
        if(bytes_received == 0)
            std::cout << "User shut the connection (FD: " << fd << ")" << std::endl;
        else
            std::cerr << "Error: Reading (FD: " << fd << ")" << std::endl;
        clientRemover(fd);
        return;
    }

    // gelen veriyi o client'in kisisel bufferina eklememiz gerekiyor
    _clients[fd]->appendBuffer(std::string(buffer, bytes_received));

    // EVRENSEL BUFFER KONTROLU (Mac ve Ubuntu Uyumu)
    std::string &client_buffer = _clients[fd]->getBuffer();
    size_t pos;

    // \n gorene kadar ara (Mac sadece \n yollar, Ubuntu \r\n yollar)
    while((pos = client_buffer.find('\n')) != std::string::npos){
        
        // \n'e kadar olan kismi kesiyoruz
        std::string command = client_buffer.substr(0, pos);

        // Eger komutun sonunda \r kalmissa onu da temizle
        if (command.length() > 0 && command[command.length() - 1] == '\r') {
            command.erase(command.length() - 1);
        }

        // test mesaji (silinebilir)
        std::cout << "Command [FD " << fd << "]: " << command << std::endl;

        // komutu islemek uzere parsere gonderiyoruz
        parseCommand(fd, command);

        if(_clients.count(fd) == 0)
            return;

        // islenen kismi ve \n karakterini (pos + 1) bufferindan siliyoruz
        _clients[fd]->eraseBuffer(pos + 1);
        client_buffer = _clients[fd]->getBuffer(); // referansi guncelliyoruz
    }
}

void Server::parseCommand(int fd, std::string command){
    std::vector<std::string> args;
    size_t pos = 0;
    std::string token;

    // stringi bosluklara gore parcalama
    while((pos = command.find(' ')) != std::string::npos){
        token = command.substr(0, pos);
        if(!token.empty())
            args.push_back(token);
        command.erase(0, pos + 1);
    }
    if(!command.empty())
        args.push_back(command);

    // hic bir sey gelmediyse cikma islemi
    if(args.empty()) return;
    
    std::string cmd = args[0]; // ilk kelime komutun kendisidir

    // komut ismini buyuk harfe ceviriyoruz (nick -> NICK, ping -> PING uyumu icin)
    for (size_t i = 0; i < cmd.length(); ++i) {
        cmd[i] = std::toupper(cmd[i]);
    }

    args.erase(args.begin()); // ilk kelimeyi listeden cikar geriye sadece parametreler kalsin

    if(_commands.find(cmd) != _commands.end()){
        // eger tanimliysa o komuta karsilik gelen fonksiyonu cagir
        (this->*_commands[cmd])(fd, args);
    }else
        std::cout << "Unknown command from FD " << fd << ": " << cmd << std::endl;
}

void    Server::cmdNick(int fd, std::vector<std::string> args){
    if (!_clients[fd]->isPassSet()){
        sendNumeric(fd, "464", "Password required first!");
        return ;
    }
    if(args.empty()){
        sendNumeric(fd, "431", "No nickname given");
        return;
    }

    std::string newNick = args[0];

    // nick kullanimda mi kontrol ediyoruz
    for(std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it){
        if(it->second->getNickname() == newNick && it->first != fd){
            sendNumeric(fd, "433", newNick + " Nickname is already in use"); // ':' karakterini kaldirdik
            return;
}
    }

    _clients[fd]->setNickname(newNick);
    _clients[fd]->setNickSet(true);
    std::cout << "FD " << fd << " set nickname to: " << newNick << std::endl;

    // kayit kontrolu : user'dan once nick girdi mi? ikisi de tamamsa kayit biter
    if(_clients[fd]->isUserSet() && !_clients[fd]->isRegistered()){
        _clients[fd]->setRegistered(true);
        std::cout << "FD " << fd << " is fully registered to the server!" << std::endl;
        sendNumeric(fd, "001", "Welcome to the ft_irc Network!");
    }
}

void    Server::cmdUser(int fd, std::vector<std::string> args){
    if (!_clients[fd]->isPassSet()){
        sendNumeric(fd, "464", "Password required first!");
        return;
    }
    if (_clients[fd]->isRegistered()){
        sendNumeric(fd, "462", "You may not reregister");
        return;
    }
    if(args.size() < 4){
        sendNumeric(fd, "461", "USER :Not enough parameters");
        return;
    }

    // username ve realname set etme
    _clients[fd]->setUsername(args[0]);

    std::string realname = args[3];
    if (realname[0] == ':')
        realname.erase(0, 1);
    _clients[fd]->setRealname(realname);

    _clients[fd]->setUserSet(true);
    std::cout << "FD " << fd << " sent USER command." << std::endl;

    // kayit kontrolu : kullanici user'dan once nick girmis mi?
    if(_clients[fd]->isNickSet() && !_clients[fd]->isRegistered()){
        _clients[fd]->setRegistered(true);
        std::cout << "FD " << fd << " is fully registered to the server!" << std::endl;
        sendNumeric(fd, "001", "Welcome to the ft_irc Network!");
    }
}

void    Server::initCommands(){
    // hangi komut geldiginde hangi fonksiyonun calisacagini ekliyoruz
    _commands["NICK"] = &Server::cmdNick;
    _commands["USER"] = &Server::cmdUser;
    _commands["PASS"] = &Server::cmdPass;
    _commands["PING"] = &Server::cmdPing;
}

void    Server::sendMessage(int fd, std::string message){
    message += "\r\n";
    // send fonksiyonu ile veriyi fd uzerinden netcat/irc istemcisine gondericez
    if(send(fd, message.c_str(), message.size(), 0) == -1){
        std::cerr << "Error: Cannot send message to FD " << fd << std::endl;
    }
}

void    Server::cmdPass(int fd, std::vector<std::string> args){
    if(args.empty()){
        std::cout << "Argument has to be something!" << std::endl;
        return;
    }
    if(args[0] == _password){
        _clients[fd]->setPassSet(true);
        std::cout << "correct password" << std::endl;
    }
    else{
        std::cout << "Wrong Password!"  << std::endl;
        sendNumeric(fd, "464", "Password incorrect");
        clientRemover(fd);
    }
}

void    Server::cmdPing(int fd, std::vector<std::string> args){
    if(args.empty()){
        std::cout << "There is no parameter!" << std::endl;
        return ;
    }
    std::string pong_reply = "PONG " + args[0];
    sendMessage(fd, pong_reply);
    std::cout << "Ping received" << std::endl;
}

void    Server::sendNumeric(int fd, std::string numeric, std::string message){
    // kullanicinin anlik nickname bilgisini aliyoruz
    std::string nick = _clients[fd]->getNickname();
    
    std::string formatted_msg = ":ircserv " + numeric + " " + nick + " :" + message;
    sendMessage(fd, formatted_msg);
}