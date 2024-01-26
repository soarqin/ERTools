#pragma once

#define WIN32_LEAN_AND_MEAN
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#undef WIN32_LEAN_AND_MEAN

SDL_Texture *loadTexture(SDL_Renderer *renderer, const unsigned char *buffer, int len);
