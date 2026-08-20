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
            if (_fds[i].revents & POLLIN) {
                if (_fds[i].fd == _serverFd) {
                    acceptNewClient();
                } else {
                    int currentFd = _fds[i].fd;
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
    if (it == _clients.end()) return;
    it->second->appendWrite(message + "\r\n");
    setPollOut(fd, true);
}