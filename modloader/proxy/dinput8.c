#include "dinput8.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

struct dinput8_dll {
    HMODULE dll;
    FARPROC OrignalDirectInput8Create;
    FARPROC OrignalDllCanUnloadNow;
    FARPROC OrignalDllGetClassObject;
    FARPROC OrignalDllRegisterServer;
    FARPROC OrignalDllUnregisterServer;
    FARPROC OrignalGetdfDIJoystick;
} dinput8;

#if defined(__cplusplus)
extern "C" {
#endif

void *dinput8_FakeDirectInput8Create() { return (void *)dinput8.OrignalDirectInput8Create(); }
void *dinput8_FakeDllCanUnloadNow() { return (void *)dinput8.OrignalDllCanUnloadNow(); }
void *dinput8_FakeDllGetClassObject() { return (void *)dinput8.OrignalDllGetClassObject(); }
void *dinput8_FakeDllRegisterServer() { return (void *)dinput8.OrignalDllRegisterServer(); }
void *dinput8_FakeDllUnregisterServer() { return (void *)dinput8.OrignalDllUnregisterServer(); }
void *dinput8_FakeGetdfDIJoystick() { return (void *)dinput8.OrignalGetdfDIJoystick(); }

bool load_dinput8_proxy() {
    wchar_t path[MAX_PATH], syspath[MAX_PATH];
    GetSystemDirectoryW(syspath, MAX_PATH);
    _snwprintf(path, MAX_PATH, L"%s\\dinput8.dll", syspath);
    dinput8.dll = LoadLibraryW(path);
    if (!dinput8.dll) {
        fprintf(stderr, "Cannot load original dinput8.dll library\n");
        return false;
    }
    dinput8.OrignalDirectInput8Create = GetProcAddress(dinput8.dll, "DirectInput8Create");
    dinput8.OrignalDllCanUnloadNow = GetProcAddress(dinput8.dll, "DllCanUnloadNow");
    dinput8.OrignalDllGetClassObject = GetProcAddress(dinput8.dll, "DllGetClassObject");
    dinput8.OrignalDllRegisterServer = GetProcAddress(dinput8.dll, "DllRegisterServer");
    dinput8.OrignalDllUnregisterServer = GetProcAddress(dinput8.dll, "DllUnregisterServer");
    dinput8.OrignalGetdfDIJoystick = GetProcAddress(dinput8.dll, "GetdfDIJoystick");
    return true;
}

#if defined(__cplusplus)
}
#endif
