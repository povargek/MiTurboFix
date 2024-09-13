#include "Plugin.h"
#include <memory>
#include <RakHook/rakhook.hpp>
#include <sampapi/CChat.h>
#include <sampapi/sampapi.h>


std::unique_ptr<Plugin> plugin;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    if (dwReason == DLL_PROCESS_ATTACH) {
        rakhook::samp_version();

        DisableThreadLibraryCalls(hModule);
        plugin = std::make_unique<Plugin>(hModule);
    }
    return TRUE;
}