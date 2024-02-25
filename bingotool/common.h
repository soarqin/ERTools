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

std::string UnicodeToUtf8(const std::wstring &wstr);