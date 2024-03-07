#include "common.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#undef WIN32_LEAN_AND_MEAN
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <cstdint>

#if defined(_MSC_VER)
#pragma comment(linker,"\"/manifestdependency:type='win32' \
    name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
    processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

int generate(const std::wstring &path, const std::wstring &msg, const std::wstring &output) {
    std::ifstream exe(std::filesystem::path(L"saveimporter.exe"), std::ios::binary);
    if (!exe.is_open()) {
        return -1;
    }
    std::ifstream file(std::filesystem::path(path), std::ios::binary);
    if (!file.is_open()) {
        exe.close();
        return -2;
    }
    std::ofstream out(std::filesystem::path(!output.empty() ? output.c_str() : L"output.exe"), std::ios::binary);
    if (!out.is_open()) {
        exe.close();
        file.close();
        return -3;
    }
    exe.seekg(0, std::ios::end);
    auto size = exe.tellg();
    exe.seekg(0, std::ios::beg);
    std::vector<uint8_t> data(size);
    exe.read(reinterpret_cast<char *>(data.data()), size);
    out.write(reinterpret_cast<char *>(data.data()), size);
    file.seekg(0, std::ios::end);
    size = file.tellg();
    file.seekg(0, std::ios::beg);
    data.resize(size);
    file.read(reinterpret_cast<char *>(data.data()), size);
    out.write(reinterpret_cast<char *>(data.data()), size);
    out.write(reinterpret_cast<char *>(&size), 4);
    size = int64_t(msg.length() * sizeof(wchar_t));
    out.write(reinterpret_cast<const char *>(msg.c_str()), size);
    out.write(reinterpret_cast<char *>(&size), 4);
    uint32_t magic = 0x49535245; // ERSI
    out.write(reinterpret_cast<char *>(&magic), 4);
    exe.close();
    file.close();
    out.close();
    return 0;
}

static WNDPROC prevProc = nullptr;

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_DROPFILES: {
            wchar_t path[MAX_PATH];
            if (DragQueryFileW(reinterpret_cast<HDROP>(wParam), 0, path, sizeof(path))) {
                SetWindowTextW(hwnd, path);
            }
            return 0;
        }
        default:
            return CallWindowProcW(prevProc, hwnd, message, wParam, lParam);
    }
}

INT_PTR CALLBACK DlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG: {
            // Put dialog in center of desktop
            RECT rc;
            GetWindowRect(GetDesktopWindow(), &rc);
            int x = (rc.right - rc.left) / 2;
            int y = (rc.bottom - rc.top) / 2;
            GetWindowRect(hwndDlg, &rc);
            int w = rc.right - rc.left;
            int h = rc.bottom - rc.top;
            SetWindowPos(hwndDlg, nullptr, x - w / 2, y - h / 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            auto hwnd = GetDlgItem(hwndDlg, 1001);
            // Enable drag and drop for edit control
            SetWindowLongPtrW(hwnd, GWL_EXSTYLE, GetWindowLongPtrW(hwnd, GWL_EXSTYLE) | WS_EX_ACCEPTFILES);
            prevProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc)));
            return TRUE;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hwndDlg, 0);
                    return TRUE;
                case 1002: {
                    wchar_t savepath[MAX_PATH];
                    SHGetSpecialFolderPathW(nullptr, savepath, CSIDL_APPDATA, false);
                    PathAppendW(savepath, L"EldenRing");
                    auto file = selectFile(L"选择存档文件", savepath, L"艾尔登法环存档文件|ER0000.sl2|所有文件|*.*", false);
                    if (!file.empty())
                        SetWindowTextW(GetDlgItem(hwndDlg, 1001), file.c_str());
                    return TRUE;
                }
                case 1005: {
                    auto file = selectFile(L"输出文件", L"", L"可执行文件|*.exe", false, false);
                    if (!file.empty())
                        SetWindowTextW(GetDlgItem(hwndDlg, 1004), file.c_str());
                    return TRUE;
                }
                case 1006: {
                    wchar_t path[MAX_PATH];
                    GetDlgItemTextW(hwndDlg, 1001, path, sizeof(path));
                    wchar_t msg[MAX_PATH];
                    GetDlgItemTextW(hwndDlg, 1003, msg, sizeof(msg));
                    wchar_t output[MAX_PATH];
                    GetDlgItemTextW(hwndDlg, 1004, output, sizeof(output));
                    auto res = generate(path, msg, output);
                    switch (res) {
                        case -1:
                            MessageBoxW(hwndDlg, L"无法打开程序文件", L"错误", 0);
                            break;
                        case -2:
                            MessageBoxW(hwndDlg, L"无法打开存档文件", L"错误", 0);
                            break;
                        case -3:
                            MessageBoxW(hwndDlg, L"无法创建输出文件", L"错误", 0);
                            break;
                        case 0:
                            MessageBoxW(hwndDlg, L"生成完毕", L"成功", 0);
                            break;
                    }
                    return TRUE;
                }
            }
            break;
    }
    return FALSE;
}

int wmain(int argc, wchar_t *argv[]) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    return DialogBoxW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(129), nullptr, DlgProc);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    int argc;
    wchar_t **argv = CommandLineToArgvW(lpCmdLine, &argc);
    return wmain(argc, argv);
}
