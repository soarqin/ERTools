/*
 * Copyright (C) 2024, Soar Qin<soarchin@gmail.com>

 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "steam/app.h"

#include <getopt.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <detours.h>

#include <shlwapi.h>

#define ER_APP_ID 1245620

static wchar_t full_game_path[MAX_PATH] = L"";
static wchar_t full_config_path[MAX_PATH] = L"YAERModLoader.ini";
static wchar_t full_modengine_dll[MAX_PATH] = L"YAERModLoader.dll";
static bool suspend = false;

bool parse_args(int argc, wchar_t *argv[]) {
    const struct option options[] = {
        {L"launch-target", required_argument, NULL, 't'},
        {L"game-path", required_argument, NULL, 'p'},
        {L"config", required_argument, NULL, 'c'},
        {L"modengine-dll", required_argument, NULL, 'd'},
        {L"suspend", no_argument, NULL, 's'},
        {NULL, 0, NULL, 0},
    };
    int opt;
    while ((opt = getopt_long(argc, argv, L":t:p:c:d:s", options, NULL)) != -1) {
        switch (opt) {
            case 't':
                /* we only support ER now, ignore this */
                break;
            case 'p':
                wcscpy(full_game_path, optarg);
                break;
            case 'c':
                wcscpy(full_config_path, optarg);
                break;
            case 'd':
                wcscpy(full_modengine_dll, optarg);
                break;
            case 's':
                suspend = true;
                break;
            case '?':
                fwprintf(stderr, L"bad arument: %c\n", optopt);
                return false;
            case ':':
                fwprintf(stderr, L"missing argument for : %c\n", optopt);
                return false;
            default:
                break;
        }
    }
    return true;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
/*
    wchar_t game_path[MAX_PATH];
    app_find_game_path(1245620, game_path);
*/
    STARTUPINFOW si = {};
    PROCESS_INFORMATION pi = {};
    HMODULE kernel32;
    FARPROC create_process_addr;
    char filepath[MAX_PATH];
    wchar_t game_folder[MAX_PATH];
    BOOL success;

    parse_args(__argc, __wargv);
    if (full_modengine_dll[0] == L'\0' || !PathFileExistsW(full_modengine_dll)) {
        GetModuleFileNameA(hInstance, filepath, MAX_PATH);
        PathRemoveFileSpecA(filepath);
        PathAppendA(filepath, "YAERModLoader.dll");
    } else {
        if (wcschr(full_modengine_dll, L':') == NULL && full_modengine_dll[0] != L'\\' && full_modengine_dll[0] != L'/') {
            char temp[MAX_PATH];
            GetModuleFileNameA(hInstance, filepath, MAX_PATH);
            PathRemoveFileSpecA(filepath);
            WideCharToMultiByte(CP_ACP, 0, full_modengine_dll, -1, temp, MAX_PATH, NULL, NULL);
            PathAppendA(filepath, temp);
        } else {
            WideCharToMultiByte(CP_ACP, 0, full_modengine_dll, -1, filepath, MAX_PATH, NULL, NULL);
        }
    }
    kernel32 = LoadLibraryW(L"kernel32.dll");
    create_process_addr = GetProcAddress(kernel32, "CreateProcessW");

    if (full_game_path[0] == L'\0' || !PathFileExistsW(full_game_path)) {
        app_find_game_path(ER_APP_ID, game_folder);
        PathAppendW(game_folder, L"Game");
        wcscpy(full_game_path, game_folder);
        PathAppendW(full_game_path, L"eldenring.exe");
    } else {
        wcscpy(game_folder, full_game_path);
        PathRemoveFileSpecW(game_folder);
    }
    SetEnvironmentVariableW(L"MODLOADER_CONFIG", full_config_path);
    {
        /* set SteamAppId here, to make sure the game can be launched w/o EAC */
        wchar_t app_id_str[16];
        _snwprintf(app_id_str, 16, L"%d", ER_APP_ID);
        SetEnvironmentVariableW(L"SteamAppId", app_id_str);
    }

    success = DetourCreateProcessWithDllW(
        full_game_path,
        NULL,
        NULL,
        NULL,
        FALSE,
        suspend ? CREATE_SUSPENDED : 0,
        NULL,
        game_folder,
        &si,
        &pi,
        filepath,
        (PDETOUR_CREATE_PROCESS_ROUTINEW)(create_process_addr));
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return success ? 0 : -1;
}
