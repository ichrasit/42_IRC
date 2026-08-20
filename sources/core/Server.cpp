#include "Server.hpp"

Server::Server(int port, std::string password) : _port(port), _password(password), _serverFd(-1) {
    initCommands();
}

Server::~Server() {
    closerFds();
}

void Server::serverInitializer() {
    struct sockaddr_in add;
    struct pollfd serverPoll;

    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd == -1)
        throw std::runtime_error("Socket is not created!");

    int en = 1;
    if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &en, sizeof(en)) == -1)
        throw std::runtime_error("Error : Setsockopt!");

    if (fcntl(_serverFd, F_SETFL, O_NONBLOCK) == -1)
        throw std::runtime_error("Error : fcntl!");

    add.sin_family = AF_INET;
    add.sin_addr.s_addr = INADDR_ANY;
    add.sin_port = htons(_port);

    if (bind(_serverFd, (struct sockaddr *)&add, sizeof(add)) == -1)
        throw std::runtime_error("Error Bind! Port is probably full!");

    if (listen(_serverFd, SOMAXCONN) == -1)
        throw std::runtime_error("Error : Listen");

    serverPoll.fd = _serverFd;
    serverPoll.events = POLLIN;
    serverPoll.revents = 0;
    _fds.push_back(serverPoll);

    std::cout << "Server " << _port << " is listening" << std::endl;
}

void Server::runner() {
    while (Server::_signal == false) {
        int poll_count = poll(&_fds[0], _fds.size(), -1);
        if (poll_count == -1 && Server::_signal == false)
            throw std::runtime_error("Poll error!");

        for (size_t i = 0; i < _fds.size(); i++) {
            int   currentFd = _fds[i].fd;
            short revents   = _fds[i].revents;

            if ((revents & POLLOUT) && currentFd != _serverFd) {
                flushClient(currentFd);
                if (_clients.find(currentFd) == _clients.end()) {
                    if (i > 0)
                        i--;
                    continue;
                }
            }

            if (revents & POLLIN) {
                if (currentFd == _serverFd) {
                    acceptNewClient();
                } else {
                    handleClientData(currentFd);
                    if (_clients.find(currentFd) == _clients.end()) {
                        if (i > 0)
                            i--;
                    }
                }
            }
        }
    }
}

void Server::closerFds() {
    for (size_t i = 0; i < _fds.size(); i++) {
        std::cout << "FD " << _fds[i].fd << " is closing." << std::endl;
        close(_fds[i].fd);
    }

    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        delete it->second;
    }
    _clients.clear();
    _fds.clear();

    for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
        delete it->second;
    }
    _channels.clear();

    std::cout << "All channels are deleted and memory is freed." << std::endl;
}

void Server::sendMessage(int fd, std::string message) {
    std::map<int, Client*>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return;

    it->second->appendWrite(message + "\r\n");
    setPollOut(fd, true);
}

void Server::setPollOut(int fd, bool enable) {
    for (size_t i = 0; i < _fds.size(); i++) {
        if (_fds[i].fd == fd) {
            if (enable)
                _fds[i].events = static_cast<short>(_fds[i].events | POLLOUT);
            else
                _fds[i].events = static_cast<short>(_fds[i].events & ~POLLOUT);
            return;
        }
    }
}

void Server::flushClient(int fd) {
    std::map<int, Client*>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return;

    Client* client = it->second;
    std::string& out = client->getWriteBuffer();

    if (out.empty()) {
        setPollOut(fd, false);
        if (client->isMarkedForClose())
            clientRemover(fd);
        return;
    }

    ssize_t sent = send(fd, out.c_str(), out.size(), 0);
    if (sent <= 0) {
        clientRemover(fd);
        return;
    }

    // Kismi yazma: yalnizca gercekten giden bayt kadarini tampondan sil.
    // Kalan kisim bir sonraki POLLOUT turunda gonderilir.
    out.erase(0, static_cast<size_t>(sent));

    if (out.empty()) {
        setPollOut(fd, false);
        if (client->isMarkedForClose())
            clientRemover(fd);
    }
}

// Kapatilacak istemcinin bekleyen ciktisi varsa (ornegin bir hata numerigi)
// once o gonderilir; kapatma islemi flushClient() icinde yapilir.
void Server::disconnectClient(int fd) {
    std::map<int, Client*>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return;

    if (it->second->hasPendingWrite()) {
        it->second->markForClose();
        setPollOut(fd, true);
        return;
    }
    clientRemover(fd);
}

// Channel artik ag katmanini tanimiyor; yayin sunucu uzerinden yapiliyor.
void Server::broadcastToChannel(Channel* channel, std::string message, Client* except) {
    if (!channel)
        return;

    const std::vector<Client*>& members = channel->getMembers();
    for (size_t i = 0; i < members.size(); ++i) {
        if (members[i] != except)
            sendMessage(members[i]->getFd(), message);
    }
}