#pragma once

#define WIN32_LEAN_AND_MEAN
#include <SDL3/SDL.h>
#undef WIN32_LEAN_AND_MEAN
#include <string>

SDL_Texture *loadTexture(SDL_Renderer *renderer, const unsigned char *buffer, int len);
std::string UnicodeToUtf8(const std::wstring &wstr);
std::wstring Utf8ToUnicode(const std::string &str);
