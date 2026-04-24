#include "server/server.hpp"
#include <iostream>
#include <cstdlib>
#include <csignal>

bool Server::_signal = false;

void signalHandler(int signum){
    (void)signum;
    std::cout << "\n[Signal Received] Server is shutting down" << std::endl;
    Server::_signal = true;
}

int main(int ac, char **av)
{
    if (ac != 3){
        std::cerr << "Usage : " << av[0] << " <port> <password>" << std::endl;
        return 1;
    }

    int port = std::atoi(av[1]);
    if(port < 1024 || port > 65535){
        std::cerr << "Error : The port has to 1024 between 65535!" << std::endl;
        return 1;
    }
    
    // Sinyalleri yakalama atamalari
    signal(SIGINT, signalHandler);
    signal(SIGQUIT, signalHandler);
    
    // HAYAT KURTARAN SATIR: Biri aniden çıkarsa sunucunun çökmesini engeller (Hem Mac Hem Ubuntu için)
    signal(SIGPIPE, SIG_IGN); 

    try{
        Server ircServer(port, av[2]);
        std::cout << "----- IRC SERVER IS STARTING  ------" << std::endl;

        // 1. Soket kurulumu, bind ve listen
        ircServer.serverInitializer();

        std::cout << "[INFO] Server is successfully initialized." << std::endl;
        std::cout << "[INFO] Waiting for connections and messages... (Press CTRL+C to stop)" << std::endl;

        // 2. Ana sonsuz dongu (Yeni baglantilari ve gelen mesajlari burada dinliyoruz)
        ircServer.runner();

    }  catch(const std::exception &e){
            std::cerr << "CRITICAL ERROR! : " << e.what() << std::endl;
            return 1;
    }
    
    std::cout << "SERVER IS CLOSED. SEE YOU LATER!" << std::endl;
    return 0; 
}