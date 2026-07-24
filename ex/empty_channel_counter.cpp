#include <iostream>
#include <map>
#include <string>


// diyelim ki bu fonksiyon server sinifinin bir uye fonksiyonudur.

int Server::getEmptyChannelCount(){
    int emptyCount = 0;

    std::map<std::string, Channel*>::iterator it;

    for(it = _channels.begin*(); it != _channels.end(); ++it){
        // it->first kanal adi string
        // it second kanal nesnei

        Channel* currentChannel = it->second;

        if(currentChannel != NULL && currentChannel->getMemberCount() == 0)
            emptyCount++
    }
    return emptyCount;
}

/*
    it = _channels.begin() mapin ilk elemanindan baslar
    it != channels.end son elamana kadar gider
    ++it bir sonraki node'a gecer

    currentchannel != null segfault almamak icindir. 
    
*/