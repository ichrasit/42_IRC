#include <iostream>
#include <string>

std::string extractCommand(std::string &buffer){
    size_t pos = buffer.find('\n');

    if(pos == std::string::npos)
        return "";

    std::string command = buffer.substr(0, pos);

    if(!command.empty() && command[command.length() - 1] == "\r")
        command.erase(command.length() - 1);

    buffer.erase(0, pos + 1);
    return command;
}


int main()
{
    std::string clientBuffer = "Nick user1\r\USer jogn 0 * :JOHN DOe\r\nPart";
    
    std::cout << "extracted1 : " << extractCommand(clientBuffer) << std::endl;
    std::cout << "extracted2 : " << extractCommand(clientBuffer) << std::endl; 
    std::cout << "Remaining buffer: " << clientBuffer << std::endl; // part kalicak!

}

/*
    Bu ornekte nonblocking bufferin icinden gelen ilk \n karakterine kadar olan kismi kesip alan geriye kalan bufferi guncelliyor
    
*/