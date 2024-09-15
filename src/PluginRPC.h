#pragma once

#include <RakNet/BitStream.h>
#include <string>


class PluginRPC {
public:
    static std::string DialogSaintizeListItem(std::string token, size_t maxLineLen = 0);
    static std::string DialogSaintizeList(const std::string sInputString, char delim, size_t maxLineLen = 0);

    bool ApplyAnimation(unsigned char& id, RakNet::BitStream* bs);
    bool ApplyActorAnimation(unsigned char& id, RakNet::BitStream* bs);
    bool ShowPlayerDialog(unsigned char& id, RakNet::BitStream* bs);
    bool InitMenu(unsigned char& id, RakNet::BitStream* bs);
};