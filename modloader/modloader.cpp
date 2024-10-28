/*
 * Copyright (C) 2024, Soar Qin<soarchin@gmail.com>

 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "modloader/config.h"
#include "modloader/gamehook.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    UNREFERENCED_PARAMETER(lpReserved);

    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);
            er::modloader::gConfig.load(hModule);
            er::modloader::GameHook::install();
            break;
        case DLL_PROCESS_DETACH:
            er::modloader::GameHook::uninstall();
            break;
        default:
            break;
    }

    return TRUE;
}
