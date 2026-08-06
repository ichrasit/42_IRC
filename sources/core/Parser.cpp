#include "Server.hpp"

void Server::initCommands() {
    _commands["NICK"] = &Server::cmdNick;
    _commands["USER"] = &Server::cmdUser;
    _commands["PASS"] = &Server::cmdPass;
    _commands["PING"] = &Server::cmdPing;
    _commands["PRIVMSG"] = &Server::cmdPrivmsg;
    _commands["JOIN"] = &Server::cmdJoin;
    _commands["PART"] = &Server::cmdPart;
    _commands["QUIT"] = &Server::cmdQuit;
    _commands["KICK"] = &Server::cmdKick;
    _commands["INVITE"] = &Server::cmdInvite;
    _commands["TOPIC"] = &Server::cmdTopic;
    _commands["MODE"] = &Server::cmdMode;
}

void Server::parseCommand(int fd, std::string command) {
    std::vector<std::string> args;
    size_t pos = 0;
    std::string token;

    while ((pos = command.find(' ')) != std::string::npos) {
        token = command.substr(0, pos);
        if (!token.empty())
            args.push_back(token);
        command.erase(0, pos + 1);
    }

    if (!command.empty())
        args.push_back(command);

    if (args.empty())
        return;

    std::string cmd = args[0];
    for (size_t i = 0; i < cmd.length(); ++i) {
        cmd[i] = std::toupper(cmd[i]);
    }

    if (!args.empty()) {
        args.erase(args.begin());
    }

    if (_commands.find(cmd) != _commands.end()) {
        (this->*_commands[cmd])(fd, args);
    } else {
        std::cout << "Unknown command from FD " << fd << ": " << cmd << std::endl;
    }
}