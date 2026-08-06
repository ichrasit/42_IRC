#include "Server.hpp"

void Server::sendNumeric(int fd, std::string numeric, std::string message) {
    std::string nick = _clients[fd]->getNickname();
    std::string formatted_msg = ":ircserv " + numeric + " " + nick + " :" + message;
    sendMessage(fd, formatted_msg);
}

void Server::clearEmptyChannels() {
    std::map<std::string, Channel*>::iterator it = _channels.begin();
    while (it != _channels.end()) {
        if (it->second->getMemberCount() == 0) {
            std::cout << "Channel " << it->first << " is empty and destroyed." << std::endl;
            delete it->second;
            std::map<std::string, Channel*>::iterator toErase = it;
            ++it;
            _channels.erase(toErase);
        } else {
            ++it;
        }
    }
}

void Server::help_printer(std::string target, int fd) {
    std::string botName = "Helper";
    std::string helpText = "Commands that you can use : !help, !ping, !rules";
    std::string botMsg = ":" + botName + " PRIVMSG " + target + " :" + helpText;
    
    if (target[0] == '#') {
        if (_channels.find(target) != _channels.end()) {
            _channels[target]->broadcast(botMsg, NULL);
        }
    } else {
        sendMessage(fd, botMsg);
    }
}