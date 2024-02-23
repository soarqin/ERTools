#include "common.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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
