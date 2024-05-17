#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <vector>
#undef WIN32_LEAN_AND_MEAN

#include "common.h"

void stripString(std::wstring &str) {
    str.erase(0, str.find_first_not_of(L" \t"));
    str.erase(str.find_last_not_of(L" \t") + 1);
}

template <typename T>
std::vector<T> splitString(const T &str, typename T::value_type sep) {
    std::vector<T> result;
    typename T::size_type pos1 = 0, pos2 = str.find(sep);
    while (pos2 != T::npos) {
        auto s = str.substr(pos1, pos2 - pos1);
        stripString(s);
        result.emplace_back(std::move(s));
        pos1 = pos2 + 1;
        pos2 = str.find(sep, pos1);
    }
    if (pos1 < str.length()) {
        auto s = str.substr(pos1);
        stripString(s);
        result.emplace_back(std::move(s));
    }
    return result;
}

std::wstring selectFile(const std::wstring &title, const std::wstring &defaultFolder, const std::wstring &filters, const std::wstring &defaultExt, bool folderOnly, bool openMode) {
    IFileDialog *pfd;
    std::wstring result;
    if (SUCCEEDED(CoCreateInstance(openMode ? CLSID_FileOpenDialog : CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd)))) {
        DWORD dwOptions;
        if (SUCCEEDED(pfd->GetOptions(&dwOptions))) {
            if (folderOnly) {
                dwOptions |= FOS_PICKFOLDERS;
            } else {
                dwOptions &= ~FOS_PICKFOLDERS;
            }
            if (openMode) {
                dwOptions |= folderOnly ? FOS_PATHMUSTEXIST : FOS_FILEMUSTEXIST;
            } else {
                dwOptions &= ~(FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST);
            }
            if (!folderOnly && !openMode) {
                dwOptions |= FOS_OVERWRITEPROMPT;
            } else {
                dwOptions &= ~FOS_OVERWRITEPROMPT;
            }
            pfd->SetOptions(dwOptions);
        }
        pfd->SetTitle(title.c_str());
        if (!defaultExt.empty()) pfd->SetDefaultExtension(defaultExt.c_str());
        if (!filters.empty()) {
            auto sl = splitString(filters, L'|');
            auto cnt = sl.size() / 2;
            if (cnt > 0) {
                auto *fs = new COMDLG_FILTERSPEC[cnt];
                for (size_t i = 0; i < cnt; i++) {
                    fs[i].pszName = sl[i * 2].c_str();
                    fs[i].pszSpec = sl[i * 2 + 1].c_str();
                }
                pfd->SetFileTypes(UINT(cnt), fs);
                delete[] fs;
            }
        }
        if (!defaultFolder.empty()) {
            IShellItem *pCurFolder = nullptr;
            if (SUCCEEDED(SHCreateItemFromParsingName(defaultFolder.c_str(), nullptr, IID_PPV_ARGS(&pCurFolder)))) {
                pfd->SetDefaultFolder(pCurFolder);
                pCurFolder->Release();
            }
        }
        auto ok = SUCCEEDED(pfd->Show(nullptr));
        if (ok) {
            IShellItem *psi;
            if (SUCCEEDED(pfd->GetResult(&psi))) {
                PWSTR selPath;
                if (!SUCCEEDED(psi->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &selPath))) {
                    return std::move(result);
                }
                result = selPath;
                CoTaskMemFree(selPath);
                psi->Release();
            }
        }
        pfd->Release();
    }
    return std::move(result);
}
