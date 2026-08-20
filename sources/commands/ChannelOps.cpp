#include "Server.hpp"
#include <sstream>

void Server::cmdJoin(int fd, std::vector<std::string> args) {
    if (!_clients[fd]->isRegistered()) {
        sendNumeric(fd, "451", "You have not registered");
        return;
    }

    if (args.empty()) {
        sendNumeric(fd, "461", "JOIN :Not enough parameters");
        return;
    }

    std::string chanName = args[0];
    std::string key = (args.size() > 1) ? args[1] : "";

    if (chanName[0] != '#') {
        chanName = "#" + chanName;
    }

    if (_channels.find(chanName) == _channels.end()) {
        _channels[chanName] = new Channel(chanName);
        _channels[chanName]->addMember(_clients[fd]);
        _channels[chanName]->addOperator(_clients[fd]);
    } else {
        Channel* chan = _channels[chanName];
        if (!chan->getPassword().empty() && chan->getPassword() != key) {
            sendNumeric(fd, "475", chanName + " :Cannot join channel (+k) - bad key");
            return;
        }

        if (chan->getUserLimit() > 0 && chan->getMemberCount() >= chan->getUserLimit()) {
            sendNumeric(fd, "471", chanName + " :Cannot join channel (+l) - channel is full");
            return;
        }

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
    for (size_t i = 0; i < _fds.size(); ++i) {
        int currentFd = _fds[i].fd;
        if (_clients.find(currentFd) != _clients.end()) {
            Client* cl = _clients[currentFd];
            if (chan->isMember(cl)) {
                if (!memberList.empty())
                    memberList += " ";
                if (chan->isOperator(cl))
                    memberList += "@";
                memberList += cl->getNickname();
            }
        }
    }

    sendNumeric(fd, "353", "= " + chanName + " :" + memberList);
    sendNumeric(fd, "366", chanName + " :End of /NAMES list.");
}

void Server::cmdPart(int fd, std::vector<std::string> args) {
    if (!_clients[fd]->isRegistered()) {
        sendNumeric(fd, "451", "You have not registered");
        return;
    }

    if (args.empty()) {
        sendNumeric(fd, "461", "PART :Not enough parameters");
        return;
    }

    std::string chanName = args[0];
    if (_channels.find(chanName) == _channels.end()) {
        sendNumeric(fd, "403", chanName + " :No such channel");
        return;
    }

    if (!_channels[chanName]->isMember(_clients[fd])) {
        sendNumeric(fd, "442", chanName + " :You're not on that channel");
        return;
    }

    std::string partMsg = ":" + _clients[fd]->getNickname() + " PART " + chanName;
    sendMessage(fd, partMsg);
    _channels[chanName]->broadcast(partMsg, _clients[fd]);
    _channels[chanName]->removeMember(_clients[fd]);
    _channels[chanName]->removeOperator(_clients[fd]);

    if (_channels[chanName]->getMemberCount() == 0) {
        delete _channels[chanName];
        _channels.erase(chanName);
        std::cout << "Channel " << chanName << " is empty and destroyed." << std::endl;
    }
}

void Server::cmdKick(int fd, std::vector<std::string> args) {
    if (!_clients[fd]->isRegistered()) {
        sendNumeric(fd, "451", "You have not registered");
        return;
    }

    if (args.size() < 2) {
        sendNumeric(fd, "461", "KICK :Not enough parameters");
        return;
    }

    std::string chanName = args[0];
    std::string targetNick = args[1];

    if (_channels.find(chanName) == _channels.end()) {
        sendNumeric(fd, "403", chanName + " :No such channel");
        return;
    }

    Channel* chan = _channels[chanName];
    if (!chan->isOperator(_clients[fd])) {
        sendNumeric(fd, "482", chanName + " :You're not channel operator");
        return;
    }

    Client* targetClient = NULL;
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        if (it->second->getNickname() == targetNick) {
            targetClient = it->second;
            break;
        }
    }

    if (!targetClient || !chan->isMember(targetClient)) {
        sendNumeric(fd, "441", targetNick + " " + chanName + " :They aren't on that channel");
        return;
    }

    std::string kickMsg = ":" + _clients[fd]->getNickname() + " KICK " + chanName + " " + targetNick + " :Kicked by operator";
    sendMessage(fd, kickMsg);
    chan->broadcast(kickMsg, _clients[fd]);
    chan->removeMember(targetClient);
    chan->removeOperator(targetClient);
}

void Server::cmdInvite(int fd, std::vector<std::string> args) {
    if (!_clients[fd]->isRegistered()) {
        sendNumeric(fd, "451", "You have not registered");
        return;
    }

    if (args.size() < 2) {
        sendNumeric(fd, "461", "INVITE :Not enough parameters");
        return;
    }

    std::string targetNick = args[0];
    std::string chanName = args[1];

    if (_channels.find(chanName) == _channels.end()) {
        sendNumeric(fd, "403", chanName + " :No such channel");
        return;
    }

    Channel* chan = _channels[chanName];
    if (!chan->isMember(_clients[fd])) {
        sendNumeric(fd, "442", chanName + " :You're not on that channel");
        return;
    }

    if (!chan->isOperator(_clients[fd])) {
        sendNumeric(fd, "482", chanName + " :You're not channel operator");
        return;
    }

    Client* targetClient = NULL;
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        if (it->second->getNickname() == targetNick) {
            targetClient = it->second;
            break;
        }
    }

    if (!targetClient) {
        sendNumeric(fd, "401", targetNick + " :No such nick/channel");
        return;
    }

    chan->addInvite(targetClient);
    std::string inviteMsg = ":" + _clients[fd]->getNickname() + " INVITE " + targetNick + " " + chanName;
    sendMessage(targetClient->getFd(), inviteMsg);
    sendNumeric(fd, "341", targetNick + " " + chanName);
}

void Server::cmdTopic(int fd, std::vector<std::string> args) {
    if (!_clients[fd]->isRegistered()) {
        sendNumeric(fd, "451", "You have not registered");
        return;
    }

    if (args.empty()) {
        sendNumeric(fd, "461", "TOPIC :Not enough parameters");
        return;
    }

    std::string chanName = args[0];
    if (_channels.find(chanName) == _channels.end()) {
        sendNumeric(fd, "403", chanName + " :No such channel");
        return;
    }

    Channel* chan = _channels[chanName];
    if (!chan->isMember(_clients[fd])) {
        sendNumeric(fd, "442", chanName + " :You're not on that channel");
        return;
    }

    if (args.size() == 1) {
        if (chan->getTopic().empty()) {
            sendNumeric(fd, "331", chanName + " :No topic is set");
        } else {
            sendNumeric(fd, "332", chanName + " :" + chan->getTopic());
        }
        return;
    }

    if (chan->isTopicRestricted() && !chan->isOperator(_clients[fd])) {
        sendNumeric(fd, "482", chanName + " :You're not channel operator");
        return;
    }

    std::string newTopic = "";
    for (size_t i = 1; i < args.size(); ++i) {
        if (i > 1) newTopic += " ";
        newTopic += args[i];
    }

    if (!newTopic.empty() && newTopic[0] == ':')
        newTopic.erase(0, 1);

    chan->setTopic(newTopic);
    std::string topicMsg;
    if (newTopic.empty()) {
        topicMsg = ":" + _clients[fd]->getNickname() + " TOPIC " + chanName + " :";
    } else {
        topicMsg = ":" + _clients[fd]->getNickname() + " TOPIC " + chanName + " :" + newTopic;
    }

    sendMessage(fd, topicMsg);
    chan->broadcast(topicMsg, _clients[fd]);
}

void Server::cmdMode(int fd, std::vector<std::string> args) {
    if (!_clients[fd]->isRegistered()) {
        sendNumeric(fd, "451", "You have not registered");
        return;
    }

    if (args.empty()) {
        sendNumeric(fd, "461", "MODE :Not enough parameters");
        return;
    }

    std::string chanName = args[0];
    if (_channels.find(chanName) == _channels.end()) {
        return;
    }

    Channel* chan = _channels[chanName];
    if (args.size() == 1) {
        std::string activeModes = "+";
        std::string modeParams = "";
        if (chan->isInviteOnly()) activeModes += "i";
        if (chan->isTopicRestricted()) activeModes += "t";
        if (!chan->getPassword().empty()) {
            activeModes += "k";
            modeParams += " " + chan->getPassword();
        }
        if (chan->getUserLimit() > 0) {
            activeModes += "l";
            std::stringstream ss;
            ss << chan->getUserLimit();
            modeParams += " " + ss.str();
        }
        sendNumeric(fd, "324", chanName + " " + activeModes + modeParams);
        return;
    }

    if (!chan->isOperator(_clients[fd])) {
        sendNumeric(fd, "482", chanName + " :You're not channel operator");
        return;
    }

    std::string modeStr = args[1];
    bool setFlag = true;
    for (size_t i = 0; i < modeStr.length(); ++i) {
        char c = modeStr[i];
        if (c == '+') setFlag = true;
        else if (c == '-') setFlag = false;
        else if (c == 'i') {
            chan->setInviteOnly(setFlag);
        } else if (c == 't') {
            chan->setTopicRestricted(setFlag);
        } else if (c == 'k') {
            if (setFlag && args.size() > 2)
                chan->setPassword(args[2]);
            else if (!setFlag)
                chan->setPassword("");
        } else if (c == 'l') {
            if (setFlag && args.size() > 2)
                chan->setUserLimit(std::atoi(args[2].c_str()));
            else if (!setFlag)
                chan->setUserLimit(0);
        } else if (c == 'o') {
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