#include "Plugin.h"
#include <RakHook/rakhook.hpp>
#include <RakNet/StringCompressor.h>
#include "RPCEnumerations.h"

#include <iomanip>
#include <sstream>

extern DWORD Debug::dwLogLevel;

Plugin::Plugin(HMODULE hndl) : hModule(hndl) {
    HKEY hKey;
    LONG lRes = RegOpenKeyExA(HKEY_CURRENT_USER, REG_CONFIG_TREE, 0, KEY_READ, &hKey);

    if (SUCCEEDED(lRes)) {

        DWORD dwNotifyFlagsRaw;

        if (SUCCEEDED(GetDWORDRegKey(hKey, REG_CONFIG_KEY, dwNotifyFlagsRaw, 0))) {
            Debug::dwLogLevel = dwNotifyFlagsRaw; // Set log level
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
    static bool msgShow = false;
    static DWORD64 dwTickShow = 0;

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

        dwTickShow = GetTickCount64();

        inited = true;
    }

    if (inited && GetTickCount64() > (dwTickShow + 3000) && !msgShow) { // dirty hack. Can't emulate RPC when not connected
        Plugin::AddChatMessageDebug(Debug::LogLevel::InitNotify, 0xFFFFFFFF, "MiTurboFix by ? inited. THIS MESSAGE CAN BE DISABLED. You need remove registry key:");
        Plugin::AddChatMessageDebug(Debug::LogLevel::InitNotify, 0xFFFFFFFF, "HKEY_CURRENT_USER//%s  key '%s'", REG_CONFIG_TREE, REG_CONFIG_KEY);

        msgShow = true;
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
    vsnprintf(szBuffer, 144, sFmtMessage.c_str(), args);
    va_end(args);

    std::string sMessage = std::string(szBuffer);

    RakNet::BitStream bs;
    bs.Write<std::uint32_t>(dwColor);
    bs.Write<std::uint32_t>(sMessage.size());
    bs.Write(sMessage.data(), sMessage.size());
    rakhook::emul_rpc(ScriptRPC::ClientMessage, bs);

    OutputDebugStringA(std::string(std::string("[MiTurboFix] ") + sMessage).data());
}
