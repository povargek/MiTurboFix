#include "PluginRPC.h"
#include "Plugin.h"

#include "misc.h"

#include <string>
#include <vector>
#include <numeric>
#include <sstream>
#include <utility>
#include <iomanip>
#include <iostream>


#include <RakNet/StringCompressor.h>
#include "RPCEnumerations.h"


#ifdef _DEBUG
void PrintBin(std::string str)
{
    for (char c : str) { std::cout << std::hex << std::setfill('0') << std::setw(2) << int(c); }

    std::cout << std::endl;
}
#endif

template <typename T>
std::string readWithSize(RakNet::BitStream& bs, size_t max = 0) {
    T size;
    if (!bs.Read(size) || size <= 0 || (max > 0 && size > max))
        return {};
    std::string str(size, '\0');
    bs.Read(str.data(), size);
    return str;
}

template <typename T>
void writeWithSize(RakNet::BitStream& bs, std::string_view str) {
    T size = static_cast<T>(str.size());
    bs.Write(size);
    bs.Write(str.data(), size);
}

template <typename T>
bool BitStreamIgnoreString(RakNet::BitStream& bs, size_t max = 0) {
    T size;
    if (!bs.Read(size) || size <= 0 || (max > 0 && size > max))
        return false;
    std::string str(size, '\0');
    bs.Read(str.data(), size);

    return str.length() <= max;
}

 inline std::string PluginRPC::DialogSaintizeListItem(std::string token, size_t maxLineLen) {
    size_t tokenLen = (maxLineLen > 0 && token.length() > maxLineLen) ? maxLineLen : token.length();

#ifdef _DEBUG
    if (maxLineLen > 0 && token.length() > maxLineLen) {
        Plugin::AddChatMessage(0xFFFFFFFF, __FUNCTION__ ": bad listitem len = %d", token.length());
    }
#endif

    return token.substr(0, tokenLen);
}


std::string PluginRPC::DialogSaintizeList(const std::string sInputString, char delim, size_t maxLineLen) {
    std::vector<std::string> tokens;
    std::string s(sInputString);
    std::string token;
    std::string sDelim{ delim };

    size_t pos = 0;

    while ((pos = s.find(delim)) != std::string::npos) {
        token = s.substr(0, pos);

        tokens.push_back(
            DialogSaintizeListItem(token, maxLineLen)
        );

        s.erase(0, pos + sDelim.length());
    }

    s.resize(std::strlen(s.c_str())); // https://stackoverflow.com/questions/29302073/difference-between-strlenstr-c-str-and-str-length-for-stdstring

    tokens.push_back(
        DialogSaintizeListItem(s, maxLineLen)
    );

    return std::accumulate(tokens.begin(), tokens.end(), std::string(),
        [delim](const std::string& a, const std::string& b) -> std::string {
            return a + (a.length() > 0 ? std::string{delim} : std::string{}) + b;
        });
}

bool PluginRPC::ApplyAnimation(unsigned char& id, RakNet::BitStream* bs) {
    if (id != ScriptRPC::ApplyAnimation) // RPC_ApplyPlayerAnimation
        return true;

    bs->IgnoreBits(16); // wPlayerId;

    size_t iAnimLibMaxLen = 15;
    size_t iAnimNameMaxLen = 24;

    std::string sAnimLib = readWithSize<uint8_t>(*bs, iAnimLibMaxLen);

    if (sAnimLib.length() < 1 || sAnimLib.length() > iAnimLibMaxLen) {
#ifdef _DEBUG
        Plugin::AddChatMessage(0xFFFFFFFF, __FUNCTION__ ": bad bAnimLibLength = %d", iAnimLibMaxLen);
#endif

        return false;
    }

    std::string sAnimName = readWithSize<uint8_t>(*bs, iAnimNameMaxLen);

    if (sAnimName.length() < 1 || sAnimName.length() > iAnimNameMaxLen) {
#ifdef _DEBUG
        Plugin::AddChatMessage(0xFFFFFFFF, __FUNCTION__ ": bad bAnimNameLength = %d", iAnimNameMaxLen);
#endif

        return false;
    }

    return true;
}


bool PluginRPC::ApplyActorAnimation(unsigned char& id, RakNet::BitStream* bs) {
    if (id != ScriptRPC::ApplyActorAnimation) 
        return true;

    bs->IgnoreBits(16); // wActorID;

    size_t iAnimLibMaxLen = 15;
    size_t iAnimNameMaxLen = 24;

    std::string sAnimLib = readWithSize<uint8_t>(*bs, iAnimLibMaxLen);

    if (sAnimLib.length() < 1 || sAnimLib.length() > iAnimLibMaxLen) {
#ifdef _DEBUG
        Plugin::AddChatMessage(0xFFFFFFFF, __FUNCTION__ ": bad bAnimLibLength = %d", iAnimLibMaxLen);
#endif

        return false;
    }

    std::string sAnimName = readWithSize<uint8_t>(*bs, iAnimNameMaxLen);

    if (sAnimName.length() < 1 || sAnimName.length() > iAnimNameMaxLen) {
#ifdef _DEBUG
        Plugin::AddChatMessage(0xFFFFFFFF, __FUNCTION__ ": bad bAnimNameLength = %d", iAnimNameMaxLen);
#endif

        return false;
    }

    return true;
}

bool PluginRPC::ShowPlayerDialog(unsigned char& id, RakNet::BitStream* bs) {
    if (id != ScriptRPC::ShowDialog) 
        return true;

    unsigned short sDialogID;
    unsigned char  bDialogStyle;
    std::string    title, but1, but2, text(4096, 0);

    // read
    bs->Read(sDialogID);
    bs->Read(bDialogStyle);

    if (!DialogType::IsListingStyle(bDialogStyle)) return true;

    title = readWithSize<unsigned char>(*bs);
    but1 = readWithSize<unsigned char>(*bs);
    but2 = readWithSize<unsigned char>(*bs);
    StringCompressor::Instance()->DecodeString(text.data(), 4096, bs);

    text = DialogSaintizeList(text, '\n', 128);

    text.resize(4096);

    // write
    bs->Reset();
    bs->Write(sDialogID);
    bs->Write(bDialogStyle);
    writeWithSize<unsigned char>(*bs, title);
    writeWithSize<unsigned char>(*bs, but1);
    writeWithSize<unsigned char>(*bs, but2);
    StringCompressor::Instance()->EncodeString(text.data(), 4096, bs);
    return true;

}