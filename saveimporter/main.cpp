#include "savefile.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#undef WIN32_LEAN_AND_MEAN
#include <fstream>
#include <cstdint>

#define MSGBOX_CAPTION L"法环存档导入"

int wmain(int argc, wchar_t *argv[]) {
    // Unused argc, argv
    (void)argc;
    (void)argv;
    CoInitialize(nullptr);

    wchar_t savepath[MAX_PATH];
    SHGetSpecialFolderPathW(nullptr, savepath, CSIDL_APPDATA, false);
    PathAppendW(savepath, L"EldenRing");
    IFileDialog *pfd;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd))))
    {
        DWORD dwOptions;
        if (SUCCEEDED(pfd->GetOptions(&dwOptions)))
        {
            pfd->SetOptions(dwOptions | FOS_PICKFOLDERS);
        }
        pfd->SetTitle(L"选择艾尔登法环存档目录(请选择数字ID子目录)");
        IShellItem *pCurFolder = nullptr;
        if (SUCCEEDED(SHCreateItemFromParsingName(savepath, NULL, IID_PPV_ARGS(&pCurFolder))))
        {
            pfd->SetDefaultFolder(pCurFolder);
            pCurFolder->Release();
        }
        if (SUCCEEDED(pfd->Show(nullptr)))
        {
            IShellItem *psi;
            if (SUCCEEDED(pfd->GetResult(&psi)))
            {
                PWSTR selPath;
                if(!SUCCEEDED(psi->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &selPath)))
                {
                    MessageBoxW(nullptr, L"GetIDListName() failed", MSGBOX_CAPTION, 0);
                }
                lstrcpyW(savepath, selPath);
                CoTaskMemFree(selPath);
                psi->Release();
            }
        }
        pfd->Release();
    }
    wchar_t testPath[MAX_PATH];
    lstrcpyW(testPath, savepath);
    PathRemoveFileSpecW(testPath);
    if (lstrcmpiW(PathFindFileNameW(testPath), L"EldenRing") != 0) {
        MessageBoxW(nullptr, L"选择了无效的存档目录", MSGBOX_CAPTION, 0);
        return -1;
    }
    auto *uidStr = PathFindFileNameW(savepath);
    wchar_t *endptr;
    uint64_t uid = std::wcstoull(uidStr, &endptr, 10);
    if (endptr && *endptr != L'\0') {
        MessageBoxW(nullptr, L"选择了无效的存档目录", MSGBOX_CAPTION, 0);
        return -1;
    }
    PathAppendW(savepath, L"ER0000.sl2");
    if (PathFileExistsW(savepath)) {
        if (MessageBoxW(nullptr, L"是否确认覆盖当前存档文件？(如有必要请确认已经备份了当前存档)", MSGBOX_CAPTION, MB_YESNO) == IDNO) {
            return -1;
        }
    }

    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, sizeof(path));
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return -1;
    }
    file.seekg(-4, std::ios::end);
    int size;
    file.read(reinterpret_cast<char *>(&size), 4);
    file.seekg(-size - 4, std::ios::end);
    std::string data;
    data.resize(size);
    file.read(data.data(), size);
    SaveFile savefile(data, savepath);
    savefile.resign(uid);
    MessageBoxW(nullptr, L"存档生成完毕", MSGBOX_CAPTION, 0);
    CoUninitialize();
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    int argc;
    wchar_t **argv = CommandLineToArgvW(lpCmdLine, &argc);
    return wmain(argc, argv);
}
