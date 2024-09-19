#include "Plugin.h"
#include <RakHook/rakhook.hpp>
#include <RakNet/StringCompressor.h>
#include "RPCEnumerations.h"

#include <iomanip>
#include <sstream>

#include <Shlobj.h>

extern DWORD Debug::dwLogLevel;
extern std::wstring Debug::sLogFilePath;


Plugin::Plugin(HMODULE hndl) : hModule(hndl) {
    HKEY hKey;
    LONG lRes = RegOpenKeyExA(HKEY_CURRENT_USER, REG_CONFIG_TREE, 0, KEY_READ, &hKey);

    if (SUCCEEDED(lRes)) {

        DWORD dwNotifyFlagsRaw;

        if (SUCCEEDED(GetDWORDRegKey(hKey, REG_CONFIG_KEY, dwNotifyFlagsRaw, 0))) {
            Debug::dwLogLevel = dwNotifyFlagsRaw; // Set log level
        }

        if (Debug::dwLogLevel & Debug::LogLevel::LogToFile) {
            std::wstring sLogFilePathRaw;

            if (SUCCEEDED(GetStringRegKeyW(hKey, REG_LOG_FILE_PATH_KEY, sLogFilePathRaw, LOG_FILE_PATH_DEFAULT))) {
                Debug::sLogFilePath = sLogFilePathRaw;
            }

            std::wstring sLogFolderPathRaw;

            if (SUCCEEDED(GetStringRegKeyW(hKey, REG_LOG_FILE_DIR_KEY, sLogFolderPathRaw, LOG_FOLDER_DEFAULT))) {
                Debug::sLogFolderPath = sLogFolderPathRaw;
            }
        }
    }

    InitConsole();

    using namespace std::placeholders;
    hookCTimerUpdate.set_cb(std::bind(&Plugin::mainloop, this, _1));
    hookCTimerUpdate.install();

    hookCMessages_AddBigMessageHooked.set_cb(std::bind(&Plugin::CMessages_AddBigMessageHooked, this, _1, _2, _3, _4));
    hookCMessages_AddBigMessageHooked.install();
}

void Plugin::mainloop(const decltype(hookCTimerUpdate)& hook) {
    static bool inited = false;

    if (!inited && rakhook::initialize()) {
        // GTA SA Patches what required SA:MP
        InstallPatchAddHospitalRestartPoint();

        // SAMP Hooks

        using namespace std::placeholders;
        StringCompressor::AddReference();

        rakhook::on_receive_rpc += std::bind(&PluginRPC::ApplyAnimation, &RPC, _1, _2);
        rakhook::on_receive_rpc += std::bind(&PluginRPC::ApplyActorAnimation, &RPC, _1, _2);
        rakhook::on_receive_rpc += std::bind(&PluginRPC::ShowPlayerDialog, &RPC, _1, _2);
        rakhook::on_receive_rpc += std::bind(&PluginRPC::InitMenu, &RPC, _1, _2);

        Plugin::AddChatMessageDebug(Debug::LogLevel::InitNotify, 0xFFFFFFFF, "MiTurboFix by ? inited. THIS MESSAGE CAN BE DISABLED. You need remove registry key:");
        Plugin::AddChatMessageDebug(Debug::LogLevel::InitNotify, 0xFFFFFFFF, "HKEY_CURRENT_USER//%s  key '%s'", REG_CONFIG_TREE, REG_CONFIG_KEY);

        inited = true;
    }


    return hook.get_trampoline()();
}

void Plugin::CMessages_AddBigMessageHooked(const decltype(hookCMessages_AddBigMessageHooked)& hook, 
    char* text, uint32_t duration, uint16_t style)
{
    if (style > 6) {
        Plugin::AddChatMessageDebug(Debug::LogLevel::DetectNotify, 0xFFFFFFFF, __FUNCTION__ ": GameText bad style = %d, strlen = %d", style, strlen(text));

        return;
    }

    return hook.get_trampoline()(text, duration, style);
}


/**
* SetSpawnInfo buffer overflow fix (not installing until SAMP is unavailable, for maybe not break single-player mode)
*
* int __cdecl CRestart::AddHospitalRestartPoint(RwV3D *point, float angle, int town)
* mov     CRestarts__m_dwHospitalRestartCount, cx
*
* =>
*
* nop (7 bytes)
*/
void Plugin::InstallPatchAddHospitalRestartPoint()
{
    MemSet(reinterpret_cast<void*>(0x460773), 0x90, 0x7);
}

void Plugin::MemSet(LPVOID lpAddr, int iVal, size_t dwSize)
{
    DWORD oldProtection = PAGE_EXECUTE_READWRITE;
    VirtualProtect(lpAddr, dwSize, oldProtection, &oldProtection);
    memset(lpAddr, iVal, dwSize);
    VirtualProtect(lpAddr, dwSize, oldProtection, &oldProtection);
}

/// <summary>
/// DEBUG Helpers
/// </summary>
/// 
/// 
LONG Plugin::GetDWORDRegKey(HKEY hKey, const std::string& strValueName, DWORD& nValue, DWORD nDefaultValue)
{
    nValue = nDefaultValue;
    DWORD dwBufferSize(sizeof(DWORD));
    DWORD nResult(0);
    LONG nError = ::RegQueryValueExA(hKey,
        strValueName.c_str(),
        0,
        NULL,
        reinterpret_cast<LPBYTE>(&nResult),
        &dwBufferSize);
    if (ERROR_SUCCESS == nError)
    {
        nValue = nResult;
    }
    return nError;
}

LONG Plugin::GetStringRegKeyA(HKEY hKey, const std::string& strValueName, std::string& strValue, const std::string& strDefaultValue)
{
    strValue = strDefaultValue;
    CHAR szBuffer[512];
    DWORD dwBufferSize = sizeof(szBuffer);
    ULONG nError;
    nError = RegQueryValueExA(hKey, strValueName.c_str(), 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
    if (ERROR_SUCCESS == nError)
    {
        strValue = szBuffer;
    }
    return nError;
}

LONG Plugin::GetStringRegKeyW(HKEY hKey, const std::wstring& strValueName, std::wstring& strValue, const std::wstring& strDefaultValue)
{
    strValue = strDefaultValue;
    WCHAR szBuffer[512];
    DWORD dwBufferSize = sizeof(szBuffer);
    ULONG nError;
    nError = RegQueryValueExW(hKey, strValueName.c_str(), 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
    if (ERROR_SUCCESS == nError)
    {
        strValue = szBuffer;
    }
    return nError;
}

void Plugin::PrintBin(std::string str)
{
#ifdef _DEBUG
    for (char c : str) { std::cout << std::hex << std::setfill('0') << std::setw(2) << int(c); }

    std::cout << std::endl;
#endif
}

/// <summary>
/// Add a debug chat message + WinDbg Message. Displayed ONLY IF you add a registry key:
/// HKEY_CURRENT_USER\SOFTWARE\SAMP     Key 'MiNotifyLevelFlags' DWORD
/// LogLevel::Disabled - Logging disabled
/// LogLevel::InitNotify - Show only message about plugin loaded
/// LogLevel::DetectNotify - Log all detections
/// </summary>
/// <param name="dwLevel">Message Log Level</param>
/// <param name="dwColor">Message Color</param>
/// <param name="sFmtMessage">Message to format</param>
/// <param name="">Args...</param>
void Plugin::AddChatMessageDebug(Debug::LogLevel dwLevel, std::uint32_t dwColor, std::string sFmtMessage, ...)
{
    if (!rakhook::initialized)
        return;

    if (!(dwLevel & Debug::dwLogLevel))
        return;

    char szBuffer[144];

    memset(szBuffer, 0x0, sizeof(szBuffer));

    va_list args;
    va_start(args, sFmtMessage);
    vsnprintf(szBuffer, sizeof(szBuffer), sFmtMessage.c_str(), args);
    va_end(args);

    std::string sMessage = std::string(szBuffer);

    auto sampVersion = rakhook::samp_version();

    if (sampVersion != rakhook::samp_ver::unknown) {
        LPVOID lpChatRef = *(LPVOID**)(rakhook::samp_addr(MemAddr::ChatRef[(int) sampVersion]));
        ((void(__thiscall*)(LPVOID, DWORD, const char*))(rakhook::samp_addr(MemAddr::AddToChatMessage[(int)sampVersion])))(lpChatRef, dwColor, sMessage.data());
    }

    /*RakNet::BitStream bs;
    bs.Write<std::uint32_t>(dwColor);
    bs.Write<std::uint32_t>(sMessage.size());
    bs.Write(sMessage.data(), sMessage.size());
    rakhook::emul_rpc(ScriptRPC::ClientMessage, bs);*/

    if (!(dwLevel & Debug::LogLevel::InitNotify)) {
        if (Debug::dwLogLevel & Debug::LogLevel::LogToFile) {
            LogToFile(sMessage);
        }
    }

    AddDebugMessageA(std::string(std::string("[MiTurboFix] ") + sMessage).data());
}

void Plugin::AddDebugMessageA(std::string sFmtMessage, ...) // UNICODE
{
    const char* const szFormat = sFmtMessage.data();

    va_list vaArgs;
    va_start(vaArgs, sFmtMessage);

    va_list vaCopy;
    va_copy(vaCopy, vaArgs);
    const int iLen = std::vsnprintf(NULL, 0, szFormat, vaCopy);
    va_end(vaCopy);

    std::vector<char> zc(iLen + 1);
    std::vsnprintf(zc.data(), zc.size(), szFormat, vaArgs);
    va_end(vaArgs);

    auto sMessage = std::string(zc.data(), zc.size());

    OutputDebugStringA(sMessage.data());

    #ifdef _DEBUG
        printf("%s\n", sMessage.data());
    #endif // _DEBUG
}

void Plugin::AddDebugMessageW(std::wstring sFmtMessage, ...) // UNICODE
{
    const wchar_t* zcFormat = sFmtMessage.data();

    va_list vaArgs;
    va_start(vaArgs, sFmtMessage);

    va_list vaCopy;
    va_copy(vaCopy, vaArgs);
    const int iLen = std::vswprintf(NULL, 0, zcFormat, vaCopy);
    va_end(vaCopy);

    std::vector<wchar_t> zc(iLen + 1);
    std::vswprintf(zc.data(), zc.size(), zcFormat, vaArgs);
    va_end(vaArgs);


    std::wstring sMessage = std::wstring(zc.data(), zc.size());

    OutputDebugStringW(sMessage.data());

    #ifdef _DEBUG
        wprintf(L"%s\n", sMessage.data());
    #endif // _DEBUG
}

void Plugin::LogToFile(std::string message) {
    if (!(Debug::dwLogLevel & Debug::LogLevel::LogToFile))
        return;

    time_t now = time(0);

    tm stTimeInfo;
    stTimeInfo = *localtime(&now);

    auto sTime = std::get_time(&stTimeInfo, "%d-%m-%Y  %H:%M:%S");

    std::wstring sResultFolder;

    if (Debug::sLogFolderPath.length() <= 0) { // AppData
        WCHAR appdata[MAX_PATH] = { 0 };

        if (!SUCCEEDED(SHGetFolderPathAndSubDirW(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, LOG_APPDATA_SUBFOLDER_DEFAULT, appdata))) {
            OutputDebugStringW(L"[MiTurboFix] Can't retreive AppData folder");
            AddDebugMessageW(L"[MiTurboFix] Can't retreive AppData folder. Error = %d", GetLastError());
            return;
        }

        sResultFolder = appdata;
    } else {
        sResultFolder = Debug::sLogFolderPath;
    }
    
    std::filesystem::path path = std::filesystem::path(sResultFolder) / Debug::sLogFilePath;

    std::ofstream ofs(path, std::ios_base::out | std::ios_base::app);
    
    if (ofs.is_open()) {
        ofs << sTime << '\t' << message << '\n';
        ofs.close();
    }
}