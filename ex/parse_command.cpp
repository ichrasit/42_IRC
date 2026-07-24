#include <iostream>
#include <vector>
#include <string>

std::vector<std::string> parseInput(std::string cmd){
    std::vector<std::string> tokens;
    size_t pos = 0;

    while((pos = cmd.find(' ')) != std::string::npos){
        std::string token = cmd.substr(0, pos);
        if(!token.empty())
            token.push_back(token);
        cmd.erase(0, pos + 1);
    }
    if(!cmd.empty())
        tokens.push_back(cmd);
    return tokens;
}

int main()
{
    std::string test = "PRIVMSG #general :Hello World!";
    std::vector<std::string> res = parseInput(test);

    for(size_t i = 0; i < res.size(); i++)
        std::cout << "arg " << i << ": " << res[i] << std::endl;
}