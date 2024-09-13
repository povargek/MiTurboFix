#include "PluginRPC.h"
#include <string>
#include <vector>
#include <numeric>
#include <sstream>
#include <utility>
#include <iomanip>
#include <iostream>

#include <sampapi/CChat.h>
#include <sampapi/CDialog.h>
#include <RakNet/StringCompressor.h>


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

std::string DialogSaintizeList(const std::string sInputString, char delim, size_t maxLineLen = 0) {
    std::vector<std::string> tokens;
    std::istringstream iss(sInputString);

    for (std::string token; std::getline(iss, token, delim); ) {
        token.erase(std::remove(token.begin(), token.end(), '\r'), token.end());

        size_t tokenLen = (maxLineLen > 0 && token.length() > maxLineLen) ? maxLineLen : token.length();

        tokens.push_back(
            token.substr(0, tokenLen)
        );
    }

    return std::accumulate(tokens.begin(), tokens.end(), std::string(),
        [delim](const std::string& a, const std::string& b) -> std::string {
            return a + (a.length() > 0 ? std::string{delim} : std::string{}) + b;
        });
}

bool PluginRPC::ApplyAnimation(unsigned char& id, RakNet::BitStream* bs) {
    if (id != 86) // RPC_ClientMessage
        return true;

    bs->IgnoreBits(16); // wPlayerId;

    size_t iAnimLibMaxLen = 15;
    size_t iAnimNameMaxLen = 24;

    std::string sAnimLib = readWithSize<uint8_t>(*bs, iAnimLibMaxLen);

    if (sAnimLib.length() < 1 || sAnimLib.length() > iAnimLibMaxLen) {
#ifdef _DEBUG
        sampapi::v037r1::RefChat()->AddMessage(0xFFFFFFFF, "bad bAnimLibLength");
#endif

        return false;
    }

    std::string sAnimName = readWithSize<uint8_t>(*bs, iAnimNameMaxLen);

    if (sAnimName.length() < 1 || sAnimName.length() > iAnimNameMaxLen) {
#ifdef _DEBUG
        sampapi::v037r1::RefChat()->AddMessage(0xFFFFFFFF, "bad bAnimNameLength");
#endif

        return false;
    }

    return true;
}




bool PluginRPC::ShowPlayerDialog(unsigned char& id, RakNet::BitStream* bs) {
    if (id != 61) // RPC_ShowDialog
        return true;

    unsigned short sDialogID;
    unsigned char  bDialogStyle;
    std::string    title, but1, but2, text(4096, 0);

    // read
    bs->Read(sDialogID);
    bs->Read(bDialogStyle);

    if (bDialogStyle != sampapi::v037r1::CDialog::DIALOG_LIST
        && bDialogStyle != sampapi::v037r1::CDialog::DIALOG_TABLIST
        && bDialogStyle != sampapi::v037r1::CDialog::DIALOG_HEADERSLIST
        ) return true;

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