#pragma once

#include <RakNet/BitStream.h>

class PluginRPC {
public:
    bool onServerMessage(unsigned char& id, RakNet::BitStream* bs);
    bool ApplyAnimation(unsigned char& id, RakNet::BitStream* bs);
    bool ShowPlayerDialog(unsigned char& id, RakNet::BitStream* bs);
};