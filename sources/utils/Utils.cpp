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
