#pragma once

#include "PluginRPC.h"
#include <kthook/kthook.hpp>


using CTimerProto = void( __cdecl* )();
using CMessagesProto = void(__cdecl*)(char* text, uint32_t duration, uint16_t style);

constexpr auto REG_CONFIG_TREE                  = "SOFTWARE\\SAMP";
constexpr auto REG_CONFIG_KEY                   = "MiNotifyLevelFlags";
constexpr auto REG_LOG_FILE_DIR_KEY             = L"MiLogDirectory";
constexpr auto REG_LOG_FILE_PATH_KEY            = L"MiLogFile";


constexpr auto LOG_FOLDER_DEFAULT               = L"";
constexpr auto LOG_FILE_PATH_DEFAULT            = L"mi_fix.log";

constexpr auto LOG_APPDATA_SUBFOLDER_DEFAULT    = L"MiTurboFix";




namespace Debug {
    enum LogLevel {
        Disabled = 0, // default
        InitNotify = 1 << 0,
        DetectNotify = 1 << 1,
        LogToFile = 1 << 2,
    };

    static DWORD dwLogLevel;
    static std::wstring sLogFolderPath;
    static std::wstring sLogFilePath;

};

class Plugin {
public:
    Plugin(HMODULE hModule);
    HMODULE hModule;

    void InstallPatchAddHospitalRestartPoint();

    void MemSet(LPVOID lpAddr, int iVal, size_t dwSize);

    static void AddChatMessageDebug(Debug::LogLevel dwLevel, std::uint32_t dwColor, std::string sFmtMessage, ...); // DEBUG

    static void AddDebugMessageA(std::string sFmtMessage, ...);
    static void AddDebugMessageW(std::wstring sFmtMessage, ...);

    static void LogToFile(std::string message);
private:
    PluginRPC RPC;
    kthook::kthook_simple<CTimerProto> hookCTimerUpdate{ reinterpret_cast<void*>(0x561B10) };
    kthook::kthook_simple<CMessagesProto> hookCMessages_AddBigMessageHooked{ reinterpret_cast<void*>(0x69F2B0) };

    void mainloop(const decltype(hookCTimerUpdate)& hook);
    void CMessages_AddBigMessageHooked(const decltype(hookCMessages_AddBigMessageHooked)& hook, char* text, uint32_t duration, uint16_t style);


/// <summary>
/// Debug helpers
/// </summary>

    LONG GetDWORDRegKey(HKEY hKey, const std::string& strValueName, DWORD& nValue, DWORD nDefaultValue);
    LONG GetStringRegKeyA(HKEY hKey, const std::string& strValueName, std::string& strValue, const std::string& strDefaultValue);
    LONG GetStringRegKeyW(HKEY hKey, const std::wstring& strValueName, std::wstring& strValue, const std::wstring& strDefaultValue);

    void PrintBin(std::string str);

    inline void InitConsole()
    {
#ifdef _DEBUG
        AllocConsole();
        SetConsoleOutputCP(1251);

        FILE* fDummy;
        freopen_s(&fDummy, "CONIN$", "r", stdin);
        freopen_s(&fDummy, "CONOUT$", "w", stderr);
        freopen_s(&fDummy, "CONOUT$", "w", stdout);
#endif
    }
};

namespace MemAddr {
    constexpr std::uintptr_t ChatRef[] = {
        0x21A0E4,
        0x21A0EC,
        0x26E8C8,
        0x26E9F8,
        0x26EB80,
        0x2ACA10
    };

    constexpr std::uintptr_t AddToChatMessage[] = {
        0x645A0,
        0x64670,
        0x679F0,
        0x68130,
        0x68170,
        0x67BE0
    };
};

