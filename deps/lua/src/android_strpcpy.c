

#if __ANDROID_API_LEVEL__ < 21

#include <string.h>

char * stpcpy(char * restrict dst, const char * restrict src) 
{
    const size_t length = strlen(src);
    memcpy(dst, src, length + 1);
    return dst + length;
}

#endif