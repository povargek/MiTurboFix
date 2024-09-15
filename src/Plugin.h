#pragma once

#include "PluginRPC.h"
#include <kthook/kthook.hpp>


using CTimerProto = void( __cdecl* )();
using CMessagesProto = void(__cdecl*)(char* text, uint32_t duration, uint16_t style);


class Plugin {
public:
    Plugin(HMODULE hModule);
    HMODULE hModule;

    void InstallPatchAddHospitalRestartPoint();

    void MemSet(LPVOID lpAddr, int iVal, size_t dwSize);

    static void AddChatMessageDebug(std::uint32_t dwColor, std::string sFmrMessage, ...); // DEBUG
private:
    PluginRPC RPC;
    kthook::kthook_simple<CTimerProto> hookCTimerUpdate{ reinterpret_cast<void*>(0x561B10) };
    kthook::kthook_simple<CMessagesProto> hookCMessages_AddBigMessageHooked{ reinterpret_cast<void*>(0x69F2B0) };

    void mainloop(const decltype(hookCTimerUpdate)& hook);
    void CMessages_AddBigMessageHooked(const decltype(hookCMessages_AddBigMessageHooked)& hook, char* text, uint32_t duration, uint16_t style);


/// <summary>
/// Debug helpers
/// </summary>

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
