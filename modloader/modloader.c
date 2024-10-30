/*
 * Copyright (C) 2024, Soar Qin<soarchin@gmail.com>

 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "modloader/config.h"
#include "modloader/gamehook.h"
#include "modloader/mod.h"

#include "proxy/winhttp.h"
#include "proxy/dxgi.h"
#include "proxy/dinput8.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    UNREFERENCED_PARAMETER(lpReserved);

    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);
            load_winhttp_proxy();
            load_dxgi_proxy();
            load_dinput8_proxy();
#if !defined(NDEBUG)
            AllocConsole();
            freopen("CONOUT$", "w", stdout);
            freopen("CONOUT$", "w", stderr);
            freopen("CONIN$", "r", stdin);
#endif
            mods_init();
            config_load(hModule);
            gamehook_install();
            break;
        case DLL_PROCESS_DETACH:
            gamehook_uninstall();
            mods_uninit();
            break;
        default:
            break;
    }

    return TRUE;
}
