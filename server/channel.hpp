#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <vector>
#include <iostream>
#include "client.hpp"

class Channel{
    private:
        std::string _name;
        std::string _topic;
        std::string _password;
        std::vector<Client*>    _members; // kanaldaki uyeler
        std::vector<Client*>    _operators; // kanaldaki adminler
        std::vector<Client*>    _invited; // davet edilenler listesi

        // modlar
        bool    _inviteOnly;   // +i
        bool    _topicRestricted; // +t
        size_t  _userLimit;    // +l (0 ise limit yok)

    public:
        Channel(std::string name);
        ~Channel();

        // getter setter
        std::string getName() const;
        std::string getTopic() const;
        void    setTopic(std::string topic);
        std::string getPassword() const;
        void    setPassword(std::string password);

        // uye yonetimi
        void    addMember(Client* client);
        void    removeMember(Client* client);
        bool    isMember(Client* client) const;

        // operator yonetimi
        void    addOperator(Client* client);
        void    removeOperator(Client* client);
        bool    isOperator(Client* client) const;

        // davet yonetimi
        void    addInvite(Client* client);
        bool    isInvited(Client* client) const;

        // mod getter ve setterlari
        bool    isInviteOnly() const { return _inviteOnly; }
        void    setInviteOnly(bool status) { _inviteOnly = status; }

        bool    isTopicRestricted() const { return _topicRestricted; }
        void    setTopicRestricted(bool status) { _topicRestricted = status; }

        size_t  getUserLimit() const { return _userLimit; }
        void    setUserLimit(size_t limit) { _userLimit = limit; }

        size_t  getMemberCount() const { return _members.size(); }
        const std::vector<Client*>& getMembers() const;

        // kanala mesaj yayinlama fonksiyonu
        void    broadcast(std::string message, Client* sender);
};

#endif