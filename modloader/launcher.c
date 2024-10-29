/*
 * Copyright (C) 2024, Soar Qin<soarchin@gmail.com>

 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>

#include "detours.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    STARTUPINFOW si = {};
    PROCESS_INFORMATION pi = {};
    HMODULE kernel32 = LoadLibraryW(L"kernel32.dll");
    FARPROC create_process_addr = GetProcAddress(kernel32, "CreateProcessW");
    char filepath[MAX_PATH];
    BOOL success;
    GetModuleFileNameA(hInstance, filepath, MAX_PATH);
    PathRemoveFileSpecA(filepath);
    PathAppendA(filepath, "ERModLoader.dll");

    success = DetourCreateProcessWithDllW(
        L"E:\\Games\\ELDEN RING\\1.16\\Game\\eldenring.exe",
        NULL,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        L"E:\\Games\\ELDEN RING\\1.16\\Game",
        &si,
        &pi,
        filepath,
        (PDETOUR_CREATE_PROCESS_ROUTINEW)(create_process_addr));
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return success ? 0 : -1;
}
