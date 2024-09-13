#include "Plugin.h"
#include <sampapi/CChat.h>
#include <RakHook/rakhook.hpp>
#include <RakNet/StringCompressor.h>

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


namespace samp = sampapi::v037r1;

Plugin::Plugin(HMODULE hndl) : hModule(hndl) {
    alloc_console();
    using namespace std::placeholders;
    hookCTimerUpdate.set_cb(std::bind(&Plugin::mainloop, this, _1));
    hookCTimerUpdate.install();
}

void Plugin::mainloop(const decltype(hookCTimerUpdate)& hook) {
    static bool inited = false;
    if (!inited && rakhook::initialize()) {
        using namespace std::placeholders;
        StringCompressor::AddReference();
        rakhook::on_receive_rpc += std::bind(&PluginRPC::ApplyAnimation, &RPC, _1, _2);
        rakhook::on_receive_rpc += std::bind(&PluginRPC::ShowPlayerDialog, &RPC, _1, _2);

        inited = true;
    }
    return hook.get_trampoline()();
}
