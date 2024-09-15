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

