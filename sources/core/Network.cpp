#include "Server.hpp"

void Server::acceptNewClient() {
    struct sockaddr_in client_add;
    socklen_t client_len = sizeof(client_add);

    int client_fd = accept(_serverFd, (struct sockaddr *)&client_add, &client_len);
    if (client_fd == -1) {
        std::cerr << "Error: Cannot accept client!" << std::endl;
        return;
    }

    if (fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1) {
        std::cerr << "Error: fcntl failed for client!" << std::endl;
        close(client_fd);
        return;
    }

    struct pollfd client_poll;
    client_poll.fd = client_fd;
    client_poll.events = POLLIN;
    client_poll.revents = 0;
    _fds.push_back(client_poll);

    _clients[client_fd] = new Client(client_fd);
    std::cout << "New client connected! FD: " << client_fd << std::endl;
}

void Server::handleClientData(int fd) {
    char buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));

    ssize_t bytes_received = recv(fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        if (bytes_received == 0)
            std::cout << "User shut the connection (FD: " << fd << ")" << std::endl;
        else
            std::cerr << "Error: Reading (FD: " << fd << ")" << std::endl;
        clientRemover(fd);
        return;
    }

    _clients[fd]->appendBuffer(std::string(buffer, bytes_received));
    if (_clients[fd]->getBuffer().size() > 2048) {
        std::cerr << "Buffer overflow protection: Closing FD " << fd << std::endl;
        clientRemover(fd);
        return;
    }

    std::string &client_buffer = _clients[fd]->getBuffer();
    size_t pos;

    while ((pos = client_buffer.find('\n')) != std::string::npos) {
        std::string command = client_buffer.substr(0, pos);
        if (command.length() > 0 && command[command.length() - 1] == '\r') {
            command.erase(command.length() - 1);
        }

        parseCommand(fd, command);
        if (_clients.count(fd) == 0)
            return;

        _clients[fd]->eraseBuffer(pos + 1);

        // Istemci kapatilmak uzere isaretlendiyse kalan komutlari isleme.
        if (_clients[fd]->isMarkedForClose())
            return;

        client_buffer = _clients[fd]->getBuffer();
    }
}

void Server::clientRemover(int fd) {
    if (_clients.count(fd)) {
        Client* client = _clients[fd];
        for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
            Client* promoted = it->second->removeMember(client);
            it->second->removeOperator(client);
            if (promoted) {
                std::string opMsg = ":ircserv MODE " + it->second->getName() + " +o " + promoted->getNickname();
                broadcastToChannel(it->second, opMsg, NULL);
            }
        }
    }

    close(fd);

    for (size_t i = 0; i < _fds.size(); i++) {
        if (_fds[i].fd == fd) {
            _fds.erase(_fds.begin() + i);
            break;
        }
    }

    if (_clients.count(fd)) {
        delete _clients[fd];
        _clients.erase(fd);
    }

    clearEmptyChannels();
    std::cout << "FD " << fd << " left on the server and removed." << std::endl;
}