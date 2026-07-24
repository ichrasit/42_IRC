#include "server.hpp"

Server::Server(int port, std::string password) : _port(port), _password(password), _serverFd(-1){
    initCommands(); // komut haritasi
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
    add.sin_port = htons(_port); // port'u internet byte sirasina cevir

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
            if(_fds[i].revents * POLLIN){
                if(_fds[i].fd == _serverFd)
                    acceptNewClient();
                else{
                    int currentFd = _fds[i].fd;
                    handleClientData(currentFd);

                    // eger clientremover cagrildiysa ve fd listeden silindiyse indeksi geri cek
                    if(_clients.find(currentFd) == _clients.end())
                        i--;
                }
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
    if (_clients.count(fd)) {
        Client* client = _clients[fd];
        for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
            it->second->removeMember(client);
            it->second->removeOperator(client);
        }
    }

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

    if (it->second->getMemberCount() == 0){
        delete it->second;
        std::map<std::string, Channel*>::iterator toErase = it;
        --it;
        _channels.erase(toErase);
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

    if(_clients[fd]->getBuffer().size() > 2048) {
        std::cerr << "Buffer overflow protection: Closing FD " << fd << std::endl;
        clientRemover(fd);
        return;
    }

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

    // komut ismini buyuk harfe ceviriyoruz
    for (size_t i = 0; i < cmd.length(); ++i) {
        cmd[i] = std::toupper(cmd[i]);
    }

    if(!args.empty()){
        args.erase(args.begin());
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
            sendNumeric(fd, "433", newNick + " Nickname is already in use");
            return;
        }
    }

    _clients[fd]->setNickname(newNick);
    _clients[fd]->setNickSet(true);
    std::cout << "FD " << fd << " set nickname to: " << newNick << std::endl;

    std::string oldNick = _clients[fd]->getNickname();
    std::string newNick = args[0];

    _clients[fd]->setNickname(newNick);
    _clients[fd]->setNickSet(true);

    // eger kullanici zaten kayitliysa ve nick degistirdiyse kanallara yayinla

    if(_clients[fd]->isRegistered()){
            std::string nickMsg = ":" + oldNick + " NICK :" + newNick;
            sendMessage(fd, nickMsg);

            for(std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it){
                if(it->second->isMember(_clients[fd]))
                    it->second->broadcast(nickMsg, _clients[fd]);
            }
    }
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
    _commands["PRIVMSG"] = &Server::cmdPrivmsg;
    _commands["JOIN"] = &Server::cmdJoin;
    _commands["PART"] = &Server::cmdPart;
    _commands["QUIT"] = &Server::cmdQuit;
    _commands["KICK"] = &Server::cmdKick;
    _commands["INVITE"] = &Server::cmdInvite;
    _commands["TOPIC"] = &Server::cmdTopic;
    _commands["MODE"] = &Server::cmdMode;
}

void    Server::sendMessage(int fd, std::string message){
    message += "\r\n";
    if(send(fd, message.c_str(), message.size(), 0) == -1){
        std::cerr << "Error: Cannot send message to FD " << fd << std::endl;
    }
}

void    Server::cmdPass(int fd, std::vector<std::string> args){
    if(args.empty()){
        sendNumeric(fd, "461", "PASS :Not enough parameters");
        return;
    }

    // zaten kayitliysa tekrar pass atmaya gerek yok
    if(_clients[fd]->isRegistered()){
        sendNumeric(fd, "462", "You may not reregister");
        return;
    }
    if(args[0] == _password){
        _clients[fd]->setPassSet(true);
        std::cout << "FD " << fd << " provided correct password." << std::endl;

    }
    else{
        sendNumeric(fd, "464", "Password Incorrect");
        std::cout << "FD " << fd << " provided wrong password. Disconnecting." << std::endl;
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

void    Server::cmdPrivmsg(int fd, std::vector<std::string> args){
    if (!_clients[fd]->isRegistered()) {
        sendNumeric(fd, "451", "You have not registered");
        return;
    }
    if (args.empty()) {
        sendNumeric(fd, "411", "No recipient given (PRIVMSG)");
        return;
    }
    if (args.size() < 2) {
        sendNumeric(fd, "412", "No text to send");
        return;
    }

    std::string target = args[0];
    std::string message = "";
    for (size_t i = 1; i < args.size(); ++i) {
        if (i > 1) message += " ";
        message += args[i];
    }

    if (message[0] == ':')
        message.erase(0, 1);

    std::string senderNick = _clients[fd]->getNickname();
    std::string fullMsg = ":" + senderNick + " PRIVMSG " + target + " :" + message;

    if (target[0] == '#') {
        if (_channels.find(target) != _channels.end()) {
            _channels[target]->broadcast(fullMsg, _clients[fd]);
        } else {
            sendNumeric(fd, "401", target + " :No such nick/channel");
        }
    } 
    else {
        bool found = false;
        for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
            if (it->second->getNickname() == target) {
                sendMessage(it->first, fullMsg);
                found = true;
                break;
            }
        }
        if (!found) {
            sendNumeric(fd, "401", target + " :No such nick/channel");
        }
    }
    std::string message = "";
    for(size_t i = 1; i < args.size(); ++i){
        if(i > 1) message += " ";
        message += args[i];
    }
    if(!message.empty() && message[0] == ":")
        message.erase(0, 1);
}

void    Server::cmdJoin(int fd, std::vector<std::string> args){
    if (!_clients[fd]->isRegistered()){
        sendNumeric(fd, "451", "You have not registered");
        return;
    }
    if (args.empty()){
        sendNumeric(fd, "461", "JOIN :Not enough parameters");
        return;
    }

    std::string chanName = args[0];
    std::string key = (args.size() > 1) ? args[1] : "";

    if (chanName[0] != '#') {
        chanName = "#" + chanName;
    }

    // kanal yoksa yeni olustur
    if (_channels.find(chanName) == _channels.end()) {
        _channels[chanName] = new Channel(chanName);
        _channels[chanName]->addMember(_clients[fd]);
        _channels[chanName]->addOperator(_clients[fd]);
    } else {
        Channel* chan = _channels[chanName];

        // 1. Şifre kontrolü (+k)
        if (!chan->getPassword().empty() && chan->getPassword() != key) {
            sendNumeric(fd, "475", chanName + " :Cannot join channel (+k) - bad key");
            return;
        }

        // 2. Limit kontrolü (+l)
        if (chan->getUserLimit() > 0 && chan->getMemberCount() >= chan->getUserLimit()) {
            sendNumeric(fd, "471", chanName + " :Cannot join channel (+l) - channel is full");
            return;
        }

        // 3. Invite-only kontrolü (+i)
        if (chan->isInviteOnly() && !chan->isInvited(_clients[fd])) {
            sendNumeric(fd, "473", chanName + " :Cannot join channel (+i) - invite only");
            return;
        }

        chan->addMember(_clients[fd]);
    }

    std::string joinMsg = ":" + _clients[fd]->getNickname() + " JOIN " + chanName;
    sendMessage(fd, joinMsg);
    _channels[chanName]->broadcast(joinMsg, _clients[fd]);

    Channel* chan = _channels[chanName];
    std::string memberList = "";

    for(size_t i = 0; i < _fds.size(); ++i){
        int currentFd = _fds[i].fd;
        if(_clients.find(currentFd) != _clients.end()){
            Client* cl = _clients[currentFd];
            if(chan->isMember(cl)){
                if(!memberList.empty())
                    memberList += " ";
                if (chan->isOperator(cl))
                    memberList += "@";
                memberList += cl->getNickname;
            }
        }
    }

    sendNumeric(fd, "353", "= " + chanName + " :" + memberList);
    sendNumeric(fd, "366", chanName + " :End of /NAMES list.");
}
void Server::cmdPart(int fd, std::vector<std::string> args){
    if (!_clients[fd]->isRegistered()){
        sendNumeric(fd, "451", "You have not registered");
        return;
    }
    if (args.empty()){
        sendNumeric(fd, "461", "PART :Not enough parameters");
        return;
    }
    std::string chanName = args[0];
    if (_channels.find(chanName) == _channels.end()){
        sendNumeric(fd, "403", chanName + " :No such channel");
        return;
    }
    if (!_channels[chanName]->isMember(_clients[fd])){
        sendNumeric(fd, "442", chanName + " :You're not on that channel");
        return;
    }

    std::string partMsg = ":" + _clients[fd]->getNickname() + " PART " + chanName;
    sendMessage(fd, partMsg);
    _channels[chanName]->broadcast(partMsg, _clients[fd]);

    _channels[chanName]->removeMember(_clients[fd]);
    _channels[chanName]->removeOperator(_clients[fd]);

    // --- FİX: Eğer kanalda üye kalmadıysa belleği temizle ve map'ten sil ---
    if (_channels[chanName]->getMemberCount() == 0) {
        delete _channels[chanName];
        _channels.erase(chanName);
        std::cout << "Channel " << chanName << " is empty and destroyed." << std::endl;
    }
}

void    Server::cmdQuit(int fd, std::vector<std::string> args){
    std::string reason = (args.empty()) ? "Client Quit" : args[0];
    if(!reason.empty() && reason[0] == ':')
        reason.erase(0, 1);
    std::string quitMsg = ":" + _clients[fd]->getNickname() + " QUIT :Quit " + reason;

    for(std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it){
        if(it->second->isMember(_clients[fd]))
            it->second->broadcast(quitMsg, _clients[fd]);
    }
    std::cout << "FD " >> fd << " sent QUIT command." << std::endl;
    clientRemover(fd);
}

void    Server::cmdKick(int fd, std::vector<std::string> args){
    if (!_clients[fd]->isRegistered()){
        sendNumeric(fd, "451", "You have not registered");
        return;
    }
    if (args.size() < 2){
        sendNumeric(fd, "461", "KICK :Not enough parameters");
        return;
    }

    std::string chanName = args[0];
    std::string targetNick = args[1];

    if (_channels.find(chanName) == _channels.end()){
        sendNumeric(fd, "403", chanName + " :No such channel");
        return;
    }

    Channel* chan = _channels[chanName];

    // atan kisi operator mu kontrolu
    if (!chan->isOperator(_clients[fd])){
        sendNumeric(fd, "482", chanName + " :You're not channel operator");
        return;
    }

    // atilacak kisi sunucuda var mi ve kanalda mi?
    Client* targetClient = NULL;
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it){
        if (it->second->getNickname() == targetNick){
            targetClient = it->second;
            break;
        }
    }

    if (!targetClient || !chan->isMember(targetClient)){
        sendNumeric(fd, "441", targetNick + " " + chanName + " :They aren't on that channel");
        return;
    }

    std::string kickMsg = ":" + _clients[fd]->getNickname() + " KICK " + chanName + " " + targetNick + " :Kicked by operator";
    sendMessage(fd, kickMsg);
    chan->broadcast(kickMsg, _clients[fd]);

    chan->removeMember(targetClient);
    chan->removeOperator(targetClient);
}

void    Server::cmdInvite(int fd, std::vector<std::string> args){
    if (!_clients[fd]->isRegistered()){
        sendNumeric(fd, "451", "You have not registered");
        return;
    }
    if (args.size() < 2){
        sendNumeric(fd, "461", "INVITE :Not enough parameters");
        return;
    }

    std::string targetNick = args[0];
    std::string chanName = args[1];

    if (_channels.find(chanName) == _channels.end()){
        sendNumeric(fd, "403", chanName + " :No such channel");
        return;
    }

    Channel* chan = _channels[chanName];

    if (!chan->isMember(_clients[fd])){
        sendNumeric(fd, "442", chanName + " :You're not on that channel");
        return;
    }

    if (!chan->isOperator(_clients[fd])){
        sendNumeric(fd, "482", chanName + " :You're not channel operator");
        return;
    }

    Client* targetClient = NULL;
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it){
        if (it->second->getNickname() == targetNick){
            targetClient = it->second;
            break;
        }
    }

    if (!targetClient){
        sendNumeric(fd, "401", targetNick + " :No such nick/channel");
        return;
    }

    // davetliler listesine ekliyoruz
    chan->addInvite(targetClient);

    std::string inviteMsg = ":" + _clients[fd]->getNickname() + " INVITE " + targetNick + " " + chanName;
    sendMessage(targetClient->getFd(), inviteMsg);
    sendNumeric(fd, "341", targetNick + " " + chanName);
}

void    Server::cmdTopic(int fd, std::vector<std::string> args){
    if (!_clients[fd]->isRegistered()){
        sendNumeric(fd, "451", "You have not registered");
        return;
    }
    if (args.empty()){
        sendNumeric(fd, "461", "TOPIC :Not enough parameters");
        return;
    }

    std::string chanName = args[0];
    if (_channels.find(chanName) == _channels.end()){
        sendNumeric(fd, "403", chanName + " :No such channel");
        return;
    }

    Channel* chan = _channels[chanName];

    if (!chan->isMember(_clients[fd])){
        sendNumeric(fd, "442", chanName + " :You're not on that channel");
        return;
    }

    // Sadece konu okuma
    if (args.size() == 1){
        if (chan->getTopic().empty()){
            sendNumeric(fd, "331", chanName + " :No topic is set");
        } else {
            sendNumeric(fd, "332", chanName + " :" + chan->getTopic());
        }
        return;
    }

    // +t modu aktifse ve degistiren kisi operator degilse engel veriyoruz
    if (chan->isTopicRestricted() && !chan->isOperator(_clients[fd])) {
        sendNumeric(fd, "482", chanName + " :You're not channel operator");
        return;
    }

    std::string newTopic = "";
    for (size_t i = 1; i < args.size(); ++i) {
        if (i > 1) newTopic += " ";
        newTopic += args[i];
    }
    if (newTopic[0] == ':') newTopic.erase(0, 1);

    chan->setTopic(newTopic);
    std::string topicMsg = ":" + _clients[fd]->getNickname() + " TOPIC " + chanName + " :" + newTopic;
    sendMessage(fd, topicMsg);
    chan->broadcast(topicMsg, _clients[fd]);
}

void    Server::cmdMode(int fd, std::vector<std::string> args){
    if (!_clients[fd]->isRegistered()){
        sendNumeric(fd, "451", "You have not registered");
        return;
    }
    if (args.empty()){
        sendNumeric(fd, "461", "MODE :Not enough parameters");
        return;
    }

    std::string chanName = args[0];
    if (_channels.find(chanName) == _channels.end()){
        return; // kullanici modlari sorgusu pas gecilir
    }

    Channel* chan = _channels[chanName];

    // Sadece mod sorgusu (MODE #kanal)
    if(args.size() == 1){
        std::string activeModes = "+";
        std::string modeParams = "";

        if(chan->isInviteOnly()) activeModes += "i";
        if(chan->isTopicRestricted()) activeModes += "t";
        if(!chan->getPassword().empty()){
            activeModes += "k";
            modeParams += " " + chan->getPassword;

        }
        if(chan->getUserLimit() > 0){
            activeModes += "l";
            std::stringstream ss;
            ss << chan->getUserLimit();
            modeParams += " " + ss.str()''
        }
        sendNumeric(fd, "324", chanName + " " + activeModes + modeParams);
        return;
    }

    // Mod degisikligi icin operator kontrolu
    if (!chan->isOperator(_clients[fd])) {
        sendNumeric(fd, "482", chanName + " :You're not channel operator");
        return;
    }

    std::string modeStr = args[1];
    bool setFlag = true; // '+' ise true, '-' ise false

    for (size_t i = 0; i < modeStr.length(); ++i) {
        char c = modeStr[i];
        if (c == '+') setFlag = true;
        else if (c == '-') setFlag = false;
        else if (c == 'i') {
            chan->setInviteOnly(setFlag);
        }
        else if (c == 't') {
            chan->setTopicRestricted(setFlag);
        }
        else if (c == 'k') {
            if (setFlag && args.size() > 2)
                chan->setPassword(args[2]);
            else if (!setFlag)
                chan->setPassword("");
        }
        else if (c == 'l') {
            if (setFlag && args.size() > 2)
                chan->setUserLimit(std::atoi(args[2].c_str()));
            else if (!setFlag)
                chan->setUserLimit(0);
        }
        else if (c == 'o') {
            if (args.size() > 2) {
                std::string targetNick = args[2];
                for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
                    if (it->second->getNickname() == targetNick) {
                        if (setFlag) chan->addOperator(it->second);
                        else chan->removeOperator(it->second);
                        break;
                    }
                }
            }
        }
    }

    std::string modeMsg = ":" + _clients[fd]->getNickname() + " MODE " + chanName + " " + modeStr;
    sendMessage(fd, modeMsg);
    chan->broadcast(modeMsg, _clients[fd]);
}

void    Server::sendNumeric(int fd, std::string numeric, std::string message){
    std::string nick = _clients[fd]->getNickname();
    std::string formatted_msg = ":ircserv " + numeric + " " + nick + " :" + message;
    sendMessage(fd, formatted_msg);
}

void    Server::clearEmptyChannels(){
    std::map<std::string, Channel*>::iterator it = _channels.begin();
    while(it != _channels.end()){
        if(it->second->getMemberCount() == 0){
            delete it-<second;
            std::map<std::string, Channel*>::iterator toErase = it;
            ++it;
            _channels.erase(toErase);
        }
        else
            ++it;
    }
}