#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <csignal>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <poll.h>
#include <arpa/inet.h>
#include <cstdlib>

#include "Client.hpp"
#include "Channel.hpp"

class Server {
    private:
        int                                     _port;
        std::string                             _password;
        int                                     _serverFd;
        
        std::vector<struct pollfd>              _fds;
        std::map<int, Client*>                  _clients;
        std::map<std::string, Channel*>         _channels;
        
        typedef void (Server::*CommandHandler)(int, std::vector<std::string>);
        std::map<std::string, CommandHandler>   _commands;

    public:
        static bool _signal;

        Server(int port, std::string password);
        ~Server();

        void    serverInitializer();
        void    runner();
        void    closerFds();

        void    acceptNewClient();
        void    handleClientData(int fd);
        void    clientRemover(int fd);
        void    clearEmptyChannels();

        void    initCommands();
        void    parseCommand(int fd, std::string command);
        
        void    sendMessage(int fd, std::string message);
        void    sendNumeric(int fd, std::string numeric, std::string message);

        void    setPollOut(int fd, bool enable);
        void    flushClient(int fd);
        void    disconnectClient(int fd);
        void    broadcastToChannel(Channel* channel, std::string message, Client* except);

        void    help_printer(std::string target, int fd);

        void    cmdPass(int fd, std::vector<std::string> args);
        void    cmdNick(int fd, std::vector<std::string> args);
        void    cmdUser(int fd, std::vector<std::string> args);
        void    cmdPing(int fd, std::vector<std::string> args);
        void    cmdPrivmsg(int fd, std::vector<std::string> args);
        void    cmdJoin(int fd, std::vector<std::string> args);
        void    cmdPart(int fd, std::vector<std::string> args);
        void    cmdQuit(int fd, std::vector<std::string> args);
        void    cmdKick(int fd, std::vector<std::string> args);
        void    cmdInvite(int fd, std::vector<std::string> args);
        void    cmdTopic(int fd, std::vector<std::string> args);
        void    cmdMode(int fd, std::vector<std::string> args);
};

#endif