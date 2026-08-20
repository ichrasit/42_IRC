#include "Server.hpp"

void Server::cmdPrivmsg(int fd, std::vector<std::string> args) {
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

    if (!message.empty() && message[0] == ':')
        message.erase(0, 1);

    std::string senderNick = _clients[fd]->getNickname();
    std::string fullMsg = ":" + senderNick + " PRIVMSG " + target + " :" + message;

    if (target[0] == '#') {
        if (_channels.find(target) != _channels.end()) {
            broadcastToChannel(_channels[target], fullMsg, _clients[fd]);
        } else {
            sendNumeric(fd, "401", target + " :No such nick/channel");
            return;
        }
    } else {
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
            return;
        }
    }

    if (message == "!help") {
        help_printer(target, fd);
    } else if (message == "!ping") {
        std::string botMsg = ":Helper PRIVMSG " + target + " :PONG! Server works perfectly.";
        if (target[0] == '#') {
            broadcastToChannel(_channels[target], botMsg, NULL);
        } else {
            sendMessage(fd, botMsg);
        }
    } else if (message == "!rules") {
        std::string botMsg = ":Helper PRIVMSG " + target + " :Rules: 1. Flood/Spam is forbidden, 2. Show some respect!";
        if (target[0] == '#') {
            broadcastToChannel(_channels[target], botMsg, NULL);
        } else {
            sendMessage(fd, botMsg);
        }
    }
}

void Server::cmdPing(int fd, std::vector<std::string> args) {
    if (args.empty()) {
        std::cout << "There is no parameter!" << std::endl;
        return;
    }

    std::string pong_reply = "PONG " + args[0];
    sendMessage(fd, pong_reply);
    std::cout << "Ping received" << std::endl;
}

void Server::cmdQuit(int fd, std::vector<std::string> args) {
    std::string reason = (args.empty()) ? "Client Quit" : args[0];
    if (!reason.empty() && reason[0] == ':')
        reason.erase(0, 1);

    std::string quitMsg = ":" + _clients[fd]->getNickname() + " QUIT :Quit " + reason;
    for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
        if (it->second->isMember(_clients[fd]))
            broadcastToChannel(it->second, quitMsg, _clients[fd]);
    }

    std::cout << "FD " << fd << " sent QUIT command." << std::endl;
    disconnectClient(fd);
}