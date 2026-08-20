#include "Server.hpp"
#include <iostream>
#include <cstdlib>
#include <csignal>
#include <cctype>

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
    if (portStr.empty()) {
        std::cerr << "Error: Port cannot be empty!" << std::endl;
        return 1;
    }

    for (size_t i = 0; i < portStr.length(); i++) {
        // isdigit'e negatif char vermek tanimsiz davranistir; unsigned char'a cevir.
        if (!std::isdigit(static_cast<unsigned char>(portStr[i]))) {
            std::cerr << "Error: Port must contain only numbers!" << std::endl;
            return 1;
        }
    }

    // atoi tasmayi bildirmez ("99999" -> htons ile bozulur). strtol + aralik kontrolu.
    long portValue = std::strtol(portStr.c_str(), NULL, 10);
    if (portValue < 1 || portValue > 65535) {
        std::cerr << "Error: Port must be between 1 and 65535!" << std::endl;
        return 1;
    }
    if (portValue < 1024) {
        std::cerr << "Warning: Ports below 1024 usually require root privileges." << std::endl;
    }

    int port = static_cast<int>(portValue);

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