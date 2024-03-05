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

std::string mergeString(const std::vector<std::string> &strs, char sep) {
    std::string result;
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

SDL_Surface *TTF_RenderUTF8_BlackOutline_Wrapped(TTF_Font *font,
                                                 const char *t,
                                                 const SDL_Color *c,
                                                 int wrapLength,
                                                 const SDL_Color *shadowColor,
                                                 int outline,
                                                 int *offset) {
    if (outline <= 0)
        return TTF_RenderUTF8_Blended_Wrapped(font, t, *c, wrapLength);

    SDL_Surface *black_letters;
    SDL_Surface *white_letters;
    SDL_Surface *bg;
    SDL_Rect dstrect;

    if (!font) {
        return nullptr;
    }

    if (!t || !c) {
        return nullptr;
    }

    if (t[0] == '\0') {
        return nullptr;
    }

    black_letters = TTF_RenderUTF8_Blended_Wrapped(font, t, *shadowColor, wrapLength);

    if (!black_letters) {
        return nullptr;
    }

    bool fullOutline = offset[0] == 0 && offset[1] == 0;
    auto olx = fullOutline ? (1 + outline * 2) : std::abs(offset[0]);
    auto oly = fullOutline ? (1 + outline * 2) : std::abs(offset[1]);
    auto sw = fullOutline ? (1 + outline * 2) : outline;
    auto sh = fullOutline ? (1 + outline * 2) : outline;
    bg = SDL_CreateSurface(black_letters->w + olx, black_letters->h + oly, SDL_PIXELFORMAT_RGBA8888);

    dstrect.y = offset[1] > 0 ? offset[1] : 0;
    dstrect.w = black_letters->w;
    dstrect.h = black_letters->h;

    for (int j = 0; j < sh; j++, dstrect.y++) {
        dstrect.x = offset[0] > 0 ? offset[0] : 0;
        for (int i = 0; i < sw; i++, dstrect.x++)
            SDL_BlitSurface(black_letters, nullptr, bg, &dstrect);
    }

    SDL_DestroySurface(black_letters);

    /* --- Put the color version of the text on top! --- */
    white_letters = TTF_RenderUTF8_Blended_Wrapped(font, t, *c, wrapLength);

    if (!white_letters) {
        return nullptr;
    }

    if (fullOutline) {
        dstrect.x = outline;
    } else if (offset[0] >= 0) {
        dstrect.x = 0;
    } else if (offset[0] < 0) {
        dstrect.x = -offset[0];
    }

    if (fullOutline) {
        dstrect.y = outline;
    } else if (offset[1] >= 0) {
        dstrect.y = 0;
    } else if (offset[1] < 0) {
        dstrect.y = -offset[1];
    }

    SDL_BlitSurface(white_letters, nullptr, bg, &dstrect);
    SDL_DestroySurface(white_letters);

    return bg;
}
