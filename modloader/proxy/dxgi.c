#include "dxgi.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

struct dxgi_dll {
    HMODULE dll;
    FARPROC OrignalApplyCompatResolutionQuirking;
    FARPROC OrignalCompatString;
    FARPROC OrignalCompatValue;
    FARPROC OrignalCreateDXGIFactory;
    FARPROC OrignalCreateDXGIFactory1;
    FARPROC OrignalCreateDXGIFactory2;
    FARPROC OrignalDXGID3D10CreateDevice;
    FARPROC OrignalDXGID3D10CreateLayeredDevice;
    FARPROC OrignalDXGID3D10GetLayeredDeviceSize;
    FARPROC OrignalDXGID3D10RegisterLayers;
    FARPROC OrignalDXGIDeclareAdapterRemovalSupport;
    FARPROC OrignalDXGIDisableVBlankVirtualization;
    FARPROC OrignalDXGIDumpJournal;
    FARPROC OrignalDXGIGetDebugInterface1;
    FARPROC OrignalDXGIReportAdapterConfiguration;
    FARPROC OrignalPIXBeginCapture;
    FARPROC OrignalPIXEndCapture;
    FARPROC OrignalPIXGetCaptureState;
    FARPROC OrignalSetAppCompatStringPointer;
    FARPROC OrignalUpdateHMDEmulationStatus;
} dxgi;

#if defined(__cplusplus)
extern "C" {
#endif

void *dxgi_FakeApplyCompatResolutionQuirking() { return (void *)dxgi.OrignalApplyCompatResolutionQuirking(); }
void *dxgi_FakeCompatString() { return (void *)dxgi.OrignalCompatString(); }
void *dxgi_FakeCompatValue() { return (void *)dxgi.OrignalCompatValue(); }
void *dxgi_FakeCreateDXGIFactory() { return (void *)dxgi.OrignalCreateDXGIFactory(); }
void *dxgi_FakeCreateDXGIFactory1() { return (void *)dxgi.OrignalCreateDXGIFactory1(); }
void *dxgi_FakeCreateDXGIFactory2() { return (void *)dxgi.OrignalCreateDXGIFactory2(); }
void *dxgi_FakeDXGID3D10CreateDevice() { return (void *)dxgi.OrignalDXGID3D10CreateDevice(); }
void *dxgi_FakeDXGID3D10CreateLayeredDevice() { return (void *)dxgi.OrignalDXGID3D10CreateLayeredDevice(); }
void *dxgi_FakeDXGID3D10GetLayeredDeviceSize() { return (void *)dxgi.OrignalDXGID3D10GetLayeredDeviceSize(); }
void *dxgi_FakeDXGID3D10RegisterLayers() { return (void *)dxgi.OrignalDXGID3D10RegisterLayers(); }
void *dxgi_FakeDXGIDeclareAdapterRemovalSupport() { return (void *)dxgi.OrignalDXGIDeclareAdapterRemovalSupport(); }
void *dxgi_FakeDXGIDisableVBlankVirtualization() { return (void *)dxgi.OrignalDXGIDisableVBlankVirtualization(); }
void *dxgi_FakeDXGIDumpJournal() { return (void *)dxgi.OrignalDXGIDumpJournal(); }
void *dxgi_FakeDXGIGetDebugInterface1() { return (void *)dxgi.OrignalDXGIGetDebugInterface1(); }
void *dxgi_FakeDXGIReportAdapterConfiguration() { return (void *)dxgi.OrignalDXGIReportAdapterConfiguration(); }
void *dxgi_FakePIXBeginCapture() { return (void *)dxgi.OrignalPIXBeginCapture(); }
void *dxgi_FakePIXEndCapture() { return (void *)dxgi.OrignalPIXEndCapture(); }
void *dxgi_FakePIXGetCaptureState() { return (void *)dxgi.OrignalPIXGetCaptureState(); }
void *dxgi_FakeSetAppCompatStringPointer() { return (void *)dxgi.OrignalSetAppCompatStringPointer(); }
void *dxgi_FakeUpdateHMDEmulationStatus() { return (void *)dxgi.OrignalUpdateHMDEmulationStatus(); }

bool load_dxgi_proxy() {
    wchar_t path[MAX_PATH], syspath[MAX_PATH];
    GetSystemDirectoryW(syspath, MAX_PATH);
    wsprintfW(path, L"%s\\dxgi.dll", syspath);
    dxgi.dll = LoadLibraryW(path);
    if (!dxgi.dll) {
        fprintf(stderr, "Cannot load original dxgi.dll library\n");
        return false;
    }
    dxgi.OrignalApplyCompatResolutionQuirking = GetProcAddress(dxgi.dll, "ApplyCompatResolutionQuirking");
    dxgi.OrignalCompatString = GetProcAddress(dxgi.dll, "CompatString");
    dxgi.OrignalCompatValue = GetProcAddress(dxgi.dll, "CompatValue");
    dxgi.OrignalCreateDXGIFactory = GetProcAddress(dxgi.dll, "CreateDXGIFactory");
    dxgi.OrignalCreateDXGIFactory1 = GetProcAddress(dxgi.dll, "CreateDXGIFactory1");
    dxgi.OrignalCreateDXGIFactory2 = GetProcAddress(dxgi.dll, "CreateDXGIFactory2");
    dxgi.OrignalDXGID3D10CreateDevice = GetProcAddress(dxgi.dll, "DXGID3D10CreateDevice");
    dxgi.OrignalDXGID3D10CreateLayeredDevice = GetProcAddress(dxgi.dll, "DXGID3D10CreateLayeredDevice");
    dxgi.OrignalDXGID3D10GetLayeredDeviceSize = GetProcAddress(dxgi.dll, "DXGID3D10GetLayeredDeviceSize");
    dxgi.OrignalDXGID3D10RegisterLayers = GetProcAddress(dxgi.dll, "DXGID3D10RegisterLayers");
    dxgi.OrignalDXGIDeclareAdapterRemovalSupport = GetProcAddress(dxgi.dll, "DXGIDeclareAdapterRemovalSupport");
    dxgi.OrignalDXGIDisableVBlankVirtualization = GetProcAddress(dxgi.dll, "DXGIDisableVBlankVirtualization");
    dxgi.OrignalDXGIDumpJournal = GetProcAddress(dxgi.dll, "DXGIDumpJournal");
    dxgi.OrignalDXGIGetDebugInterface1 = GetProcAddress(dxgi.dll, "DXGIGetDebugInterface1");
    dxgi.OrignalDXGIReportAdapterConfiguration = GetProcAddress(dxgi.dll, "DXGIReportAdapterConfiguration");
    dxgi.OrignalPIXBeginCapture = GetProcAddress(dxgi.dll, "PIXBeginCapture");
    dxgi.OrignalPIXEndCapture = GetProcAddress(dxgi.dll, "PIXEndCapture");
    dxgi.OrignalPIXGetCaptureState = GetProcAddress(dxgi.dll, "PIXGetCaptureState");
    dxgi.OrignalSetAppCompatStringPointer = GetProcAddress(dxgi.dll, "SetAppCompatStringPointer");
    dxgi.OrignalUpdateHMDEmulationStatus = GetProcAddress(dxgi.dll, "UpdateHMDEmulationStatus");
    return true;
}

#if defined(__cplusplus)
}
#endif
