#include "Server.hpp"
#include <iostream>
#include <cstdlib>
#include <csignal>

bool Server::_signal = false;

void signalHandler(int signum) {
    (void)signum;
    std::cout << "\n[Signal Received] Server is shutting down" << std::endl;
    Server::_signal = true;
}

int main(int ac, char **av) {
    if (ac != 3) {
        std::cerr << "Usage: " << av[0] << " <port> <password>" << std::endl;
        return 1;
    }

    std::string portStr = av[1];
    for (size_t i = 0; i < portStr.length(); i++) {
        if (!std::isdigit(portStr[i])) {
            std::cerr << "Error: Port must contain only numbers!" << std::endl;
            return 1;
        }
    }

    int port = std::atoi(av[1]);

    signal(SIGINT, signalHandler);
    signal(SIGQUIT, signalHandler);
    signal(SIGPIPE, SIG_IGN);

    try {
        Server ircServer(port, av[2]);
        
        std::cout << "----- IRC SERVER IS STARTING -----" << std::endl;
        ircServer.serverInitializer();
        
        std::cout << "[INFO] Server is successfully initialized." << std::endl;
        std::cout << "[INFO] Waiting for connections and messages... (Press CTRL+C to stop)" << std::endl;
        
        ircServer.runner();
    } catch (const std::exception &e) {
        std::cerr << "CRITICAL ERROR! : " << e.what() << std::endl;
        return 1;
    }

    std::cout << "SERVER IS CLOSED. SEE YOU LATER!" << std::endl;
    return 0;
}