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
        std::vector<Client*>    _members; // kanalda ki tüm üyeler
        std::vector<Client*>    _operators; // kanaldaki adminler

    public:
        Channel(std::string name);
        ~Channel();

        //getter setter
        std::string getName() const;
        std::string getTopic() const;
        void    setTopic(std::string topic);
        std::string getPassword() const;
        void    setPassword(std::string password);

        // üye yönetimi
        void    addMember(Client* client);
        void    removeMember(Client* client);
        bool    isMember(Client* client) const;

        // operator yonetimi
        void    addOperator(Client* client);
        void    removeOperator(Client* client);
        bool    isOperator(Client* client) const;
        

        // kanala mesaj yayınlama fonksiyonu

        void    broadcast(std::string message, Client* sender);
};

#endif
