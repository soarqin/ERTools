#pragma once

#define WIN32_LEAN_AND_MEAN
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <vector>
#include <string>
#undef WIN32_LEAN_AND_MEAN

SDL_Texture *loadTexture(SDL_Renderer *renderer, const unsigned char *buffer, int len);
SDL_Texture *loadTexture(SDL_Renderer *renderer, const char *filename);

void stripString(std::string &str);
std::vector<std::string> splitString(const std::string &str, char sep);
std::string mergeString(const std::vector<std::string> &strs, char sep);
void unescape(std::string &str);

std::string UnicodeToUtf8(const std::wstring &wstr);
std::wstring Utf8ToUnicode(const std::string &str);

SDL_Surface *TTF_RenderUTF8_BlackOutline_Wrapped(TTF_Font *font, const char *t, const SDL_Color *c, int wrapLength, const SDL_Color *shadowColor, int outline, int offset[2]);
