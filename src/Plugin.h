#pragma once

#include "PluginRPC.h"
#include <kthook/kthook.hpp>


using CTimerProto = void( __cdecl* )();
using CMessagesProto = void(__cdecl*)(char* text, uint32_t duration, uint16_t style);


class Plugin {
public:
    Plugin(HMODULE hModule);
    HMODULE hModule;

    static void AddChatMessage(std::uint32_t dwColor, std::string sFmrMessage, ...);
    void InstallPatchAddHospitalRestartPoint();
private:
    PluginRPC RPC;
    kthook::kthook_simple<CTimerProto> hookCTimerUpdate{ reinterpret_cast<void*>(0x561B10) };
    kthook::kthook_simple<CMessagesProto> hookCMessages_AddBigMessageHooked{ reinterpret_cast<void*>(0x69F2B0) };

    void mainloop(const decltype(hookCTimerUpdate)& hook);
    void CMessages_AddBigMessageHooked(const decltype(hookCMessages_AddBigMessageHooked)& hook, char* text, uint32_t duration, uint16_t style);

};
