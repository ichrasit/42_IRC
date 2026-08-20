#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <vector>
#include "Client.hpp"

class Channel {
    private:
        std::string             _name;
        std::string             _topic;
        std::string             _password;
        
        std::vector<Client*>    _members;
        std::vector<Client*>    _operators;
        std::vector<Client*>    _invited;
        
        bool                    _inviteOnly;
        bool                    _topicRestricted;
        size_t                  _userLimit;

    public:
        Channel(std::string name);
        ~Channel();

        std::string getName() const;
        std::string getTopic() const;
        void        setTopic(std::string topic);
        
        std::string getPassword() const;
        void        setPassword(std::string password);

        void        addMember(Client* client);
        // Uyeyi cikarir. Kanal operatorsuz kaldiysa terfi ettirilen yeni
        // operatoru dondurur, aksi halde NULL. Yayin cagiran tarafin isi.
        Client*     removeMember(Client* client);
        bool        isMember(Client* client) const;

        const std::vector<Client*>& getMembers() const { return _members; }

        void        addOperator(Client* client);
        void        removeOperator(Client* client);
        bool        isOperator(Client* client) const;

        void        addInvite(Client* client);
        bool        isInvited(Client* client) const;

        bool        isInviteOnly() const { return _inviteOnly; }
        void        setInviteOnly(bool status) { _inviteOnly = status; }

        bool        isTopicRestricted() const { return _topicRestricted; }
        void        setTopicRestricted(bool status) { _topicRestricted = status; }

        size_t      getUserLimit() const { return _userLimit; }
        void        setUserLimit(size_t limit) { _userLimit = limit; }

        size_t      getMemberCount() const { return _members.size(); }
};

#endif