#include "Server.hpp"
#include <cctype>

// RFC 2812 nick kurali: ilk karakter harf ya da ozel karakter olmali,
// devami ek olarak rakam ve '-' kabul eder. Azami uzunluk 9.
// '#', ':', ',' ve bosluk gibi karakterler protokolu bozdugu icin yasak.
#define NICK_MAX_LEN 9

static bool isValidNick(const std::string& nick) {
    if (nick.empty() || nick.length() > NICK_MAX_LEN)
        return false;

    const std::string special = "[]\\`_^{|}";

    if (!std::isalpha(static_cast<unsigned char>(nick[0]))
        && special.find(nick[0]) == std::string::npos)
        return false;

    for (size_t i = 1; i < nick.length(); ++i) {
        if (!std::isalnum(static_cast<unsigned char>(nick[i]))
            && special.find(nick[i]) == std::string::npos
            && nick[i] != '-')
            return false;
    }
    return true;
}

void Server::cmdPass(int fd, std::vector<std::string> args) {
    if (args.empty()) {
        sendNumeric(fd, "461", "PASS :Not enough parameters");
        return;
    }

    if (_clients[fd]->isRegistered()) {
        sendNumeric(fd, "462", "You may not reregister");
        return;
    }

    if (args[0] == _password) {
        _clients[fd]->setPassSet(true);
        std::cout << "FD " << fd << " provided correct password." << std::endl;
    } else {
        sendNumeric(fd, "464", "Password Incorrect");
        std::cout << "FD " << fd << " provided wrong password. Disconnecting." << std::endl;
        disconnectClient(fd);
    }
}

void Server::cmdNick(int fd, std::vector<std::string> args) {
    if (!_clients[fd]->isPassSet()) {
        sendNumeric(fd, "464", "Password required first!");
        return;
    }

    if (args.empty()) {
        sendNumeric(fd, "431", "No nickname given");
        return;
    }

    std::string newNick = args[0];

    if (!isValidNick(newNick)) {
        sendNumeric(fd, "432", newNick + " Erroneous nickname");
        return;
    }

    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        if (it->second->getNickname() == newNick && it->first != fd) {
            sendNumeric(fd, "433", newNick + " Nickname is already in use");
            return;
        }
    }

    std::string oldNick = _clients[fd]->getNickname();
    _clients[fd]->setNickname(newNick);
    _clients[fd]->setNickSet(true);
    std::cout << "FD " << fd << " set nickname to: " << newNick << std::endl;

    if (_clients[fd]->isRegistered()) {
        std::string nickMsg = ":" + oldNick + " NICK :" + newNick;
        sendMessage(fd, nickMsg);
        for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
            if (it->second->isMember(_clients[fd]))
                broadcastToChannel(it->second, nickMsg, _clients[fd]);
        }
    }

    if (_clients[fd]->isUserSet() && !_clients[fd]->isRegistered()) {
        _clients[fd]->setRegistered(true);
        std::cout << "FD " << fd << " is fully registered to the server!" << std::endl;
        sendNumeric(fd, "001", "Welcome to the ft_irc Network!");
    }
}

void Server::cmdUser(int fd, std::vector<std::string> args) {
    if (!_clients[fd]->isPassSet()) {
        sendNumeric(fd, "464", "Password required first!");
        return;
    }

    if (_clients[fd]->isRegistered()) {
        sendNumeric(fd, "462", "You may not reregister");
        return;
    }

    if (args.size() < 4) {
        sendNumeric(fd, "461", "USER :Not enough parameters");
        return;
    }

    _clients[fd]->setUsername(args[0]);
    std::string realname = args[3];
    if (!realname.empty() && realname[0] == ':')
        realname.erase(0, 1);
    
    _clients[fd]->setRealname(realname);
    _clients[fd]->setUserSet(true);
    std::cout << "FD " << fd << " sent USER command." << std::endl;

    if (_clients[fd]->isNickSet() && !_clients[fd]->isRegistered()) {
        _clients[fd]->setRegistered(true);
        std::cout << "FD " << fd << " is fully registered to the server!" << std::endl;
        sendNumeric(fd, "001", "Welcome to the ft_irc Network!");
    }
}