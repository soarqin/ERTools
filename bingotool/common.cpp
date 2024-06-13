#include "common.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_WINDOWS_UTF8
#include "stb_image.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#undef WIN32_LEAN_AND_MEAN
#if defined(max)
#undef max
#endif
#if defined(min)
#undef min
#endif

SDL_Texture *loadTexture(SDL_Renderer *renderer, const unsigned char *buffer, int len) {
    int width, height, bytesPerPixel;
    void *data = stbi_load_from_memory(buffer, len, &width, &height, &bytesPerPixel, 4);
    int pitch;
    pitch = width * 4;
    pitch = (pitch + 3) & ~3;
    SDL_Surface *surface = SDL_CreateSurfaceFrom(data, width, height, pitch, SDL_PIXELFORMAT_ABGR8888);
    if (surface == nullptr) {
        stbi_image_free(data);
        return nullptr;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    stbi_image_free(data);
    return texture;
}

SDL_Texture *loadTexture(SDL_Renderer *renderer, const char *filename) {
    int width, height, bytesPerPixel;
    void *data = stbi_load(filename, &width, &height, &bytesPerPixel, 4);
    int pitch;
    pitch = width * 4;
    pitch = (pitch + 3) & ~3;
    SDL_Surface *surface = SDL_CreateSurfaceFrom(data, width, height, pitch, SDL_PIXELFORMAT_ABGR8888);
    if (surface == nullptr) {
        stbi_image_free(data);
        return nullptr;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    stbi_image_free(data);
    return texture;
}

HBITMAP loadBitmap(const char *filename) {
    int width, height, bytesPerPixel;
    void *data = stbi_load(filename, &width, &height, &bytesPerPixel, 4);
    if (!data) return nullptr;
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    // Swap red and blue channel
    auto size = width * height;
    for (int i = 0; i < size; i++) {
        auto *p = (unsigned char*)data + i * 4;
        auto t = p[0];
        p[0] = p[2];
        p[2] = t;
    }
    HBITMAP bitmap = CreateDIBitmap(GetDC(nullptr), &bmi.bmiHeader, CBM_INIT, data, &bmi, DIB_RGB_COLORS);
    stbi_image_free(data);
    return bitmap;
}

void stripString(std::string &str) {
    str.erase(0, str.find_first_not_of(" \t"));
    str.erase(str.find_last_not_of(" \t") + 1);
}

void stripString(std::wstring &str) {
    str.erase(0, str.find_first_not_of(L" \t"));
    str.erase(str.find_last_not_of(L" \t") + 1);
}

std::vector<std::string> splitString(const std::string &str, char sep) {
    std::vector<std::string> result;
    std::string::size_type pos1 = 0, pos2 = str.find(sep);
    while (pos2 != std::string::npos) {
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

std::vector<std::wstring> splitString(const std::wstring &str, wchar_t sep) {
    std::vector<std::wstring> result;
    std::wstring::size_type pos1 = 0, pos2 = str.find(sep);
    while (pos2 != std::wstring::npos) {
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

std::string mergeString(const std::vector<std::string> &strs, char sep) {
    std::string result;
    for (auto &s: strs) {
        if (!result.empty()) result += sep;
        result += s;
    }
    return result;
}

[[maybe_unused]] std::wstring mergeString(const std::vector<std::wstring> &strs, wchar_t sep) {
    std::wstring result;
    for (auto &s: strs) {
        if (!result.empty()) result += sep;
        result += s;
    }
    return result;
}

void unescape(std::string &str) {
    for (auto ite = str.begin(); ite != str.end(); ite++) {
        if (*ite == '\\') {
            ite = str.erase(ite);
            if (ite == str.end()) break;
            switch (*ite) {
                case 'n':
                    *ite = '\n';
                    break;
                case 'r':
                    *ite = '\r';
                    break;
                case 't':
                    *ite = '\t';
                    break;
                case '0':
                    *ite = '\0';
                    break;
                case '\\':
                    break;
                default:
                    ite--;
                    break;
            }
        }
    }
}

std::string UnicodeToUtf8(const std::wstring &wstr) {
    std::string result;
    auto requiredSize = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (requiredSize > 0) {
        result.resize(requiredSize);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], requiredSize, nullptr, nullptr);
    }
    if (result.back() == 0) result.pop_back();
    return result;
}

std::wstring Utf8ToUnicode(const std::string &str) {
    std::wstring result;
    auto requiredSize = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (requiredSize > 0) {
        result.resize(requiredSize);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], requiredSize);
    }
    if (result.back() == 0) result.pop_back();
    return result;
}

std::wstring selectFile(HWND hwnd, const std::wstring &title, const std::wstring &defaultFolder, const std::wstring &filters, bool folderOnly, bool openMode) {
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
        } else {
            wchar_t name[MAX_PATH];
            GetModuleFileNameW(nullptr, name, MAX_PATH);
            PathRemoveFileSpecW(name);
            IShellItem *pCurFolder = nullptr;
            if (SUCCEEDED(SHCreateItemFromParsingName(name, nullptr, IID_PPV_ARGS(&pCurFolder)))) {
                pfd->SetDefaultFolder(pCurFolder);
                pCurFolder->Release();
            }
        }
        auto ok = SUCCEEDED(pfd->Show(hwnd));
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
