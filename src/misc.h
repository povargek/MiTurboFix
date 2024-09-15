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

    bool IsListingStyle(int iDialogType) {
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

/// <summary>
/// String helpers
/// </summary>

namespace String {
    inline bool ValidateLen(char* szBuffer, size_t maxSize) {
        return strnlen_s(szBuffer, maxSize) < maxSize;
    }
};

