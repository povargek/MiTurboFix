#pragma once

/// <summary>
/// Dialog Types ... (From https://github.com/BlastHackNet/SAMP-API)
/// </summary>
namespace DialogType  {
    enum {
        MessageBox,
        Input,
        List,
        Password,
        TabList,
        HeadersList

    };

    inline bool IsListingStyle(int iDialogType) {
        switch (iDialogType) {
            case List: case TabList: case HeadersList: 
                return true;
            default: 
                return false;
        }
    }
};

/// <summary>
/// Menu (from RakSAMP)
/// </summary>

namespace Menu {
    constexpr auto MAX_MENUS = 128;
    constexpr auto MAX_MENU_ITEMS = 12;
    constexpr auto MAX_MENU_LINE = 32;

    struct Interaction
    {
        unsigned char bMenu;
        unsigned char bRow[MAX_MENU_ITEMS];
        unsigned char bPadding[8 - ((MAX_MENU_ITEMS + 1) % 8)];
    };
};

namespace Texture {
    static const uint32_t kRwStruct = 0x01;
    static const uint32_t kRwTextureDictionary = 0x16;

    struct ChunkHeader 
    {
        uint32_t type;
        uint32_t length;
        uint32_t version;
    };
};


/// <summary>
/// String helpers
/// </summary>

namespace String {
    inline bool ValidateLen(char* szBuffer, size_t maxSize) {
        return strnlen_s(szBuffer, maxSize) < maxSize;
    }
};

