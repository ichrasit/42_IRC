#include "Server.hpp"

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