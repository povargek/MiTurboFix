#include "Plugin.h"
#include <RakHook/rakhook.hpp>
#include <RakNet/StringCompressor.h>
#include "RPCEnumerations.h"


void alloc_console() {
#ifdef _DEBUG
    AllocConsole();
    SetConsoleOutputCP(1251);

    FILE* fDummy;
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
#endif
}


Plugin::Plugin(HMODULE hndl) : hModule(hndl) {
    alloc_console();
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

        inited = true;
    }
    return hook.get_trampoline()();
}

void Plugin::CMessages_AddBigMessageHooked(const decltype(hookCMessages_AddBigMessageHooked)& hook, 
    char* text, uint32_t duration, uint16_t style)
{
    if (style > 6) {
#ifdef _DEBUG
        Plugin::AddChatMessage(0xFFFFFFFF, __FUNCTION__ ": bad style = %d, strlen = %d", style, strlen(text));
#endif
        return;
    }

    return hook.get_trampoline()(text, duration, style);
}

void Plugin::AddChatMessage(std::uint32_t dwColor, std::string sFmrMessage, ...)
{
    if (!rakhook::initialized)
        return;

    char szBuffer[144];

    memset(szBuffer, 0x0, sizeof(szBuffer));

    va_list args;
    va_start(args, sFmrMessage);
    vsnprintf(szBuffer, 144, sFmrMessage.c_str(), args);
    va_end(args);

    std::string sMessage = std::string(szBuffer);

    RakNet::BitStream bs;
    bs.Write<std::uint32_t>(dwColor);
    bs.Write<std::uint32_t>(sMessage.size());
    bs.Write(sMessage.data(), sMessage.size());
    rakhook::emul_rpc(ScriptRPC::ClientMessage, bs);
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
    memset(reinterpret_cast<void*>(0x460773), 0x90, 0x7);
}
