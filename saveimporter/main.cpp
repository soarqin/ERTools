#include "savefile.h"
#include "common.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#undef WIN32_LEAN_AND_MEAN
#include <fstream>
#include <cstdint>

#if defined(_MSC_VER)
#pragma comment(linker,"\"/manifestdependency:type='win32' \
    name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
    processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#define MSGBOX_CAPTION L"法环存档导入"

int wmain(int argc, wchar_t *argv[]) {
    // Unused argc, argv
    (void)argc;
    (void)argv;
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, sizeof(path));
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return -1;
    }
    file.seekg(-4, std::ios::end);
    uint32_t magic;
    file.read(reinterpret_cast<char *>(&magic), 4);
    if (magic != 0x49535245) {
        MessageBoxW(nullptr, L"无效的存档导入文件!", MSGBOX_CAPTION, 0);
        return -1;
    }
    file.seekg(-8, std::ios::end);
    int size;
    file.read(reinterpret_cast<char *>(&size), 4);
    file.seekg(-size - 4, std::ios::cur);
    std::wstring msg;
    msg.resize((size + 1) / 2);
    file.read(reinterpret_cast<char *>(msg.data()), size);
    file.seekg(-size, std::ios::cur);
    if (!msg.empty())
        MessageBoxW(nullptr, msg.c_str(), MSGBOX_CAPTION, 0);

    wchar_t savepath[MAX_PATH];
    SHGetSpecialFolderPathW(nullptr, savepath, CSIDL_APPDATA, false);
    PathAppendW(savepath, L"EldenRing");
    auto res = selectFile(L"选择艾尔登法环存档目录(请选择数字ID子目录)", savepath, L"", true);
    if (res.empty()) return -1;
    lstrcpyW(savepath, res.c_str());
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

    file.seekg(-4, std::ios::cur);
    file.read(reinterpret_cast<char *>(&size), 4);
    file.seekg(-size - 4, std::ios::cur);
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
