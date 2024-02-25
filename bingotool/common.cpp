#include "common.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

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

void stripString(std::string &str) {
    str.erase(0, str.find_first_not_of(" \t"));
    str.erase(str.find_last_not_of(" \t") + 1);
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
