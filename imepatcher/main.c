#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint8_t *load_from_file(const char *filename, long *file_size) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    *file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8_t *buffer = (uint8_t *)malloc(*file_size);
    if (buffer == NULL) {
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, *file_size, file);
    fclose(file);

    return buffer;
}

size_t search(const uint8_t *haystack, size_t haystack_size, const char *needle_str) {
    size_t maxlen = strlen(needle_str);
    uint8_t *needle = (uint8_t*)malloc(maxlen);
    uint8_t *mask = (uint8_t*)malloc(maxlen);
    int real_len;
    int odd;
    const uint8_t *p, *e;

    if (needle == NULL) {
        return (size_t)-1;
    }
    real_len = 0;
    odd = 0;
    while (*needle_str != 0) {
        switch (*needle_str) {
            case '?':
                needle[real_len] = 0;
                mask[real_len] = 0;
                real_len++;
                if (*(needle_str + 1) == '?')
                    needle_str++;
                break;
            case ' ':
                if (odd) {
                    real_len++;
                    odd = 0;
                }
                break;
            case '0' ... '9':
                if (odd) {
                    needle[real_len] = (needle[real_len] << 4) | (*needle_str & 0x0F);
                    mask[real_len] = (mask[real_len] << 4) | 0x0F;
                    real_len++;
                    odd = 0;
                } else {
                    needle[real_len] = *needle_str & 0x0F;
                    mask[real_len] = 0x0F;
                    odd = 1;
                }
                break;
            case 'A' ... 'F':
            case 'a' ... 'f':
                if (odd) {
                    needle[real_len] = (needle[real_len] << 4) | ((*needle_str & 0x0F) + 9);
                    mask[real_len] = (mask[real_len] << 4) | 0x0F;
                    real_len++;
                    odd = 0;
                } else {
                    needle[real_len] = (*needle_str & 0x0F) + 9;
                    mask[real_len] = 0x0F;
                    odd = 1;
                }
                break;
        }
        needle_str++;
    }
    if (odd) {
        real_len++;
    }
    if (real_len == 0) {
        free(needle);
        free(mask);
        return (size_t)-1;
    }
    p = haystack;
    e = haystack + haystack_size - real_len;
    while (p < e) {
        int i;
        for (i = 0; i < real_len; i++) {
            if ((*(p + i) & mask[i]) != needle[i]) {
                break;
            }
        }
        if (i == real_len) {
            free(mask);
            free(needle);
            return p - haystack;
        }
        p++;
    }
    free(mask);
    free(needle);
}

int main(int argc, char *argv[]) {
    const wchar_t *text[2][10] = {
        {
            L"Failed to open eldenring.exe for reading",
            L"Unable to find the pattern in eldenring.exe",
            L"Failed to open eldenring_ime.exe for writing",
            L"Successfully patched codes and generated eldenring_ime.exe",
            L"Error",
            L"Success",
        },
        {
            L"无法读取 eldenring.exe",
            L"在 eldenring.exe 中搜索不到特征码段",
            L"无法写入 eldenring_ime.exe",
            L"成功对代码打补丁并生成 eldenring_ime.exe",
            L"错误",
            L"成功",
        }
    };
    size_t offset;
    long file_size;
    uint32_t locale_id = GetUserDefaultLCID() & 0xFFFFu;
    int lng = locale_id == 0x0804u || locale_id == 0x0C04u || locale_id == 0x404u ? 1 : 0;
    uint8_t *data;
    data = load_from_file("eldenring.exe", &file_size);
    if (data == NULL) {
        MessageBoxW(NULL, text[lng][0], text[lng][4], MB_ICONERROR);
        return 1;
    }
    offset = search(data, file_size, "44 89 64 24 64 33 C9 E8");
    if (offset == (size_t)-1) {
        free(data);
        MessageBoxW(NULL, text[lng][1], text[lng][4], MB_ICONERROR);
        return 1;
    }
    memset(data + offset + 5, 0x90, 7);

    offset = search(data, file_size, "E8 ?? ?? ?? ?? 80 BF ?? 00 00 00 00 74 ?? 48 8B 05");
    if (offset != (size_t)-1) {
        memset(data + offset + 12, 0x90, 2);
    }

    {
        FILE *file = fopen("eldenring_ime.exe", "wb");
        if (file == NULL) {
            free(data);
            MessageBoxW(NULL, text[lng][2], text[lng][4], MB_ICONERROR);
            return 1;
        }
        fwrite(data, 1, file_size, file);
        fclose(file);
    }
    free(data);
    MessageBoxW(NULL, text[lng][3], text[lng][5], MB_ICONINFORMATION);
    return 0;
}
