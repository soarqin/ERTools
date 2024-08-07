#include "savefile.h"
#include "common.h"

#include <toml.hpp>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#undef WIN32_LEAN_AND_MEAN
#include <fstream>
#include <cstdint>
#include <filesystem>

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
    INITCOMMONCONTROLSEX iccex = {sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&iccex);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

#if defined(USE_CLI_ARGS)
    std::ifstream file(std::filesystem::path(argv[1]), std::ios::binary);
    if (!file.is_open()) {
        CoUninitialize();
        return -1;
    }
    auto cfg = toml::parse(file);
    file.close();
#else
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, sizeof(path));
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        CoUninitialize();
        return -1;
    }
    file.seekg(-4, std::ios::end);
    uint32_t magic;
    file.read(reinterpret_cast<char *>(&magic), 4);
    if (magic != 0x49535245) {
        MessageBoxW(nullptr, L"无效的存档导入文件!", MSGBOX_CAPTION, 0);
        CoUninitialize();
        return -1;
    }
    file.seekg(-8, std::ios::end);
    int size;
    file.read(reinterpret_cast<char *>(&size), 4);
    file.seekg(-size - 4, std::ios::cur);
    std::string td;
    td.resize(size);
    file.read(td.data(), size);
    file.seekg(-size, std::ios::cur);
    std::istringstream is(td);
    auto cfg = toml::parse(is);
#endif
    auto keepFace = toml::find_or(cfg, "KeepFace", false);
    auto patchTimeToZero = toml::find_or(cfg, "PatchTimeToZero", false);
    auto msg = toml::find_or(cfg, "Message", std::wstring());
    if (!msg.empty())
        MessageBoxW(nullptr, msg.c_str(), MSGBOX_CAPTION, 0);

    wchar_t savepath[MAX_PATH];
    SHGetSpecialFolderPathW(nullptr, savepath, CSIDL_APPDATA, false);
    PathAppendW(savepath, L"EldenRing");
    auto res = selectFile(L"选择艾尔登法环存档文件(进入数字ID子目录后选择，你需要至少运行一次游戏才能生成存档文件)", savepath, L"艾尔登法环存档文件|ER0000.sl2|所有文件|*.*", L"sl2", false);
    if (res.empty()) {
        CoUninitialize();
        return -1;
    }

    int slot = 0;
    SaveFile savefile(res);
    if (!savefile.ok()) {
        MessageBoxW(nullptr, L"无效的存档文件！", MSGBOX_CAPTION, MB_ICONERROR);
        CoUninitialize();
        return -1;
    }
#if defined(USE_CLI_ARGS)
    savefile.importFromFile(argv[2], slot, [patchTimeToZero, &savefile, slot]() {
        if (patchTimeToZero) savefile.patchSlotTime(slot, 0);
    }, keepFace);
#else
    std::vector<uint8_t> data;
    file.seekg(-4, std::ios::cur);
    file.read(reinterpret_cast<char *>(&size), 4);
    file.seekg(-size - 4, std::ios::cur);
    data.resize(size);
    file.read((char*)data.data(), size);
    file.close();

    TASKDIALOG_BUTTON buttons[10] = {};
    TASKDIALOG_BUTTON dbuttons[2] = {
        {IDCANCEL, L"取消"},
        {IDOK, L"导入"},
    };
    WCHAR names[10][32];
    int count = 0;
    savefile.listSlots(-1, [&buttons, &count, &names](int idx, const SaveSlot &s) {
        if (s.slotType != SaveSlot::Character) return;
        const auto &c = (const CharSlot &)s;
        if (!c.available) {
            return;
        }
        auto &b = buttons[count];
        b.nButtonID = idx;
        b.pszButtonText = names[count];
        lstrcpyW(names[count], L"替换 ");
        lstrcatW(names[count], c.charname.c_str());
        count++;
    });
    if (count <= 0) {
        MessageBoxW(nullptr, L"没有可用的角色槽！", MSGBOX_CAPTION, 0);
        CoUninitialize();
        return -1;
    }
    TASKDIALOGCONFIG taskDialogConfig = {sizeof(TASKDIALOGCONFIG)};
    taskDialogConfig.hInstance = GetModuleHandleW(nullptr);
    taskDialogConfig.pszWindowTitle = L"选择导入角色槽";
    taskDialogConfig.pszMainIcon = MAKEINTRESOURCEW(1);
    if (keepFace) {
        taskDialogConfig.pszContent = L"选择新角色槽会导入为新角色。\n选择现有角色槽时会替换角色并且保留捏脸，注意备份数据。";
    } else {
        taskDialogConfig.pszContent = L"选择新角色槽会导入为新角色。\n选择现有角色槽时会替换角色，注意备份数据。";
    }
    taskDialogConfig.cButtons = 2;
    taskDialogConfig.pButtons = dbuttons;
    taskDialogConfig.cRadioButtons = count;
    taskDialogConfig.pRadioButtons = buttons;
    taskDialogConfig.nDefaultRadioButton = slot;
    int sel;
    if (TaskDialogIndirect(&taskDialogConfig, &sel, &slot, nullptr) != S_OK
        || sel == IDCANCEL || slot < 0) {
        CoUninitialize();
        return -1;
    }

    savefile.importFrom(data, slot & 0x0F, [patchTimeToZero, &savefile, slot]() {
        if (patchTimeToZero) savefile.patchSlotTime(slot, 0);
    }, keepFace);
#endif
    MessageBoxW(nullptr, L"存档导入完毕", MSGBOX_CAPTION, 0);
    CoUninitialize();
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    int argc;
    wchar_t **argv = CommandLineToArgvW(lpCmdLine, &argc);
    return wmain(argc, argv);
}
