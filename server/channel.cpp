#include "channel.hpp"
#include <sys/socket.h>

Channel::Channel(std::string name) : _name(name), _topic(""), _password(""), _inviteOnly(false), _topicRestricted(true), _userLimit(0){
    std::cout << "Channel " << _name << " created." << std::endl;
}

Channel::~Channel(){}

std::string Channel::getName() const {return _name;}

std::string Channel::getTopic() const {return _topic;}

void    Channel::setTopic(std::string topic) {_topic = topic;}

std::string Channel::getPassword() const {return _password;}

void    Channel::setPassword(std::string password) {_password = password;}

void    Channel::addMember(Client* client){
    if(!isMember(client))
        _members.push_back(client);
}

void    Channel::removeMember(Client* client){
    for(std::vector<Client*>::iterator it = _members.begin(); it != _members.end(); ++it){
        if(*it == client){
            _members.erase(it);
            break;
        }
    }
    removeOperator(client);
    // eger uye kaldiysa ve hic op kalmadiysa ilk uyeyi op yap!
    if(!_members.empty() && _operators.empty()){
        addOperator(_members[0]);
        std::string msg = ":ircserv MODE " + _name + " +o " + _members[0]->getNickname();
        broadcast(msg, NULL);
    }
}

bool    Channel::isMember(Client* client) const{
    for(size_t i = 0; i < _members.size(); ++i){
        if(_members[i] == client)
            return true;
    }
    return false;
}

void    Channel::addOperator(Client* client){
    if(!isOperator(client))
        _operators.push_back(client);
}

void    Channel::removeOperator(Client* client){
    for(std::vector<Client*>::iterator it = _operators.begin(); it != _operators.end(); ++it){
        if(*it == client){
            _operators.erase(it);
            break;
        }
    }
}

bool    Channel::isOperator(Client* client) const{
    for(size_t i = 0; i < _operators.size(); ++i){
        if(_operators[i] == client)
            return true;
    }
    return false;
}

void    Channel::addInvite(Client* client){
    if(!isInvited(client))
        _invited.push_back(client);
}

bool    Channel::isInvited(Client* client) const{
    for(size_t i = 0; i < _invited.size(); ++i){
        if(_invited[i] == client)
            return true;
    }
    return false;
}

void    Channel::broadcast(std::string message, Client* sender){
    message += "\r\n";
    for(size_t i = 0; i < _members.size(); ++i){
        if(_members[i] != sender){
            send(_members[i]->getFd(), message.c_str(), message.size(), 0);
        }
    }
}