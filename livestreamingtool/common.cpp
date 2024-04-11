#include "common.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

SDL_Texture *loadTexture(SDL_Renderer *renderer, const unsigned char *buffer, int len) {
    int width, height, bytesPerPixel;
    void *data = stbi_load_from_memory(buffer, len, &width, &height, &bytesPerPixel, 4);
    int pitch;
    pitch = width * bytesPerPixel;
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
