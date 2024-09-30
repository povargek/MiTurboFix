#include "PluginRPC.h"
#include "Plugin.h"

#include <string>
#include <vector>
#include <numeric>
#include <utility>
#include <iostream>


#include <RakNet/StringCompressor.h>
#include "RPCEnumerations.h"

#include "misc.h"

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

    if (maxLineLen > 0 && token.length() > maxLineLen) {
        Plugin::AddChatMessageDebug(Debug::LogLevel::DetectNotify,
            0xFFFFFFFF, __FUNCTION__ ": bad listitem len = %d", token.length());
    }

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

    bs->ResetReadPointer();

    bs->IgnoreBits(16); // wPlayerId;

    size_t iAnimLibMaxLen = 15;
    size_t iAnimNameMaxLen = 24;

    std::string sAnimLib = readWithSize<uint8_t>(*bs, iAnimLibMaxLen);

    if (sAnimLib.length() < 1 || sAnimLib.length() > iAnimLibMaxLen) {
        Plugin::AddChatMessageDebug(Debug::LogLevel::DetectNotify,
            0xFFFFFFFF, __FUNCTION__ ": bad bAnimLibLength = %d", iAnimLibMaxLen);

        return false;
    }

    std::string sAnimName = readWithSize<uint8_t>(*bs, iAnimNameMaxLen);

    if (sAnimName.length() < 1 || sAnimName.length() > iAnimNameMaxLen) {
        Plugin::AddChatMessageDebug(Debug::LogLevel::DetectNotify,
            0xFFFFFFFF, __FUNCTION__ ": bad bAnimNameLength = %d", iAnimNameMaxLen);

        return false;
    }

    return true;
}


bool PluginRPC::ApplyActorAnimation(unsigned char& id, RakNet::BitStream* bs) {
    if (id != ScriptRPC::ApplyActorAnimation) 
        return true;

    bs->ResetReadPointer();

    bs->IgnoreBits(16); // wActorID;

    size_t iAnimLibMaxLen = 15;
    size_t iAnimNameMaxLen = 24;

    std::string sAnimLib = readWithSize<uint8_t>(*bs, iAnimLibMaxLen);

    if (sAnimLib.length() < 1 || sAnimLib.length() > iAnimLibMaxLen) {
        Plugin::AddChatMessageDebug(Debug::LogLevel::DetectNotify,
            0xFFFFFFFF, __FUNCTION__ ": bad bAnimLibLength = %d", iAnimLibMaxLen);

        return false;
    }

    std::string sAnimName = readWithSize<uint8_t>(*bs, iAnimNameMaxLen);

    if (sAnimName.length() < 1 || sAnimName.length() > iAnimNameMaxLen) {
        Plugin::AddChatMessageDebug(Debug::LogLevel::DetectNotify,
            0xFFFFFFFF, __FUNCTION__ ": bad bAnimNameLength = %d", iAnimNameMaxLen);

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

    bs->ResetReadPointer();

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

bool PluginRPC::InitMenu(unsigned char& id, RakNet::BitStream* bs) {
    if (id != ScriptRPC::InitMenu) 
        return true;

    unsigned char bMenuID;
    unsigned int bIsTwoColumns;

    char szText[Menu::MAX_MENU_LINE];

    float fX, fY;
    float fColWidth[2] = { 0.0, 0.0 };

    Menu::Interaction MenuInteraction; // enabled/disabled

    memset(&MenuInteraction, 0x0, sizeof(Menu::Interaction));

    bs->ResetReadPointer();

    bs->Read(bMenuID);

    if (bMenuID >= Menu::MAX_MENUS) {
        Plugin::AddChatMessageDebug(Debug::LogLevel::DetectNotify,
            0xFFFFFFFF, __FUNCTION__ ": bad bMenuID = %d", bMenuID);

        return false;
    }

    bs->Read(bIsTwoColumns);
    bs->Read(szText, Menu::MAX_MENU_LINE);

    if (!String::ValidateLen(szText, Menu::MAX_MENU_LINE)) {
        Plugin::AddChatMessageDebug(Debug::LogLevel::DetectNotify,
            0xFFFFFFFF, __FUNCTION__ ": bad MenuTitle length = %d", strnlen_s(szText, Menu::MAX_MENU_LINE));

        return false;
    }

    bs->Read(fX);
    bs->Read(fY);

    bs->Read(fColWidth[0]);
    if (bIsTwoColumns) bs->Read(fColWidth[1]);

    bs->Read(MenuInteraction.bMenu); 

    for (unsigned char i = 0; i < Menu::MAX_MENU_ITEMS; i++)
        bs->Read(MenuInteraction.bRow[i]);


    bs->Read(szText, Menu::MAX_MENU_LINE);

    if (!String::ValidateLen(szText, Menu::MAX_MENU_LINE)) {
        Plugin::AddChatMessageDebug(Debug::LogLevel::DetectNotify,
            0xFFFFFFFF, __FUNCTION__ ": bad MenuHeaderCol1 length = %d", strnlen_s(szText, Menu::MAX_MENU_LINE));

        return false;
    }

    unsigned char byteRowCount;
    
    if (!bs->Read(byteRowCount)) {
        Plugin::AddChatMessageDebug(Debug::LogLevel::DetectNotify,
            0xFFFFFFFF, __FUNCTION__ ": bad can't read MenuRowCountCol1");

        return false;
    }

    if (byteRowCount > Menu::MAX_MENU_ITEMS) {
        Plugin::AddChatMessageDebug(Debug::LogLevel::DetectNotify,
            0xFFFFFFFF, __FUNCTION__ ": bad MenuRowCountCol1 = %d", byteRowCount);

        return false;
    }

    for (unsigned char i = 0; i < byteRowCount; i++) {
        bs->Read(szText, Menu::MAX_MENU_LINE);

        if (!String::ValidateLen(szText, Menu::MAX_MENU_LINE)) {
            Plugin::AddChatMessageDebug(Debug::LogLevel::DetectNotify,
                0xFFFFFFFF, __FUNCTION__ ": bad MenuRowCol1(%d) length = %d", i, strnlen_s(szText, Menu::MAX_MENU_LINE));

            return false;
        }
    }

    if (bIsTwoColumns) {
        bs->Read(szText, Menu::MAX_MENU_LINE);

        if (!String::ValidateLen(szText, Menu::MAX_MENU_LINE)) {
            Plugin::AddChatMessageDebug(Debug::LogLevel::DetectNotify,
                0xFFFFFFFF, __FUNCTION__ ": bad MenuHeaderCol2 length = %d", strnlen_s(szText, Menu::MAX_MENU_LINE));

            return false;
        }

        if (!bs->Read(byteRowCount)) {
            Plugin::AddChatMessageDebug(Debug::LogLevel::DetectNotify,
                0xFFFFFFFF, __FUNCTION__ ": bad can't read MenuRowCountCol1");

            return false;
        }

        if (byteRowCount > Menu::MAX_MENU_ITEMS) {
            Plugin::AddChatMessageDebug(Debug::LogLevel::DetectNotify,
                0xFFFFFFFF, __FUNCTION__ ": bad MenuRowCountCol2 = %d", byteRowCount);

            return false;
        }

        for (unsigned char i = 0; i < byteRowCount; i++) {
            bs->Read(szText, Menu::MAX_MENU_LINE);

            if (!String::ValidateLen(szText, Menu::MAX_MENU_LINE)) {
                Plugin::AddChatMessageDebug(Debug::LogLevel::DetectNotify, 
                    0xFFFFFFFF, __FUNCTION__ ": bad MenuRowCol2(%d) length = %d", i, strnlen_s(szText, Menu::MAX_MENU_LINE));

                return false;
            }
        }
    }

    return true;
}
