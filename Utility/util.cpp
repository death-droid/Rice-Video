#include "util.h"

char* left(char* src, size_t nchars)
{
    static char dst[300];			// BUGFIX (STRMNNRM)
    strncpy(dst, src, nchars);
    dst[nchars] = 0;
    return dst;
}

char* right(char *src, size_t nchars)
{
    static char dst[300];

    size_t srclen = strlen(src);
    if (nchars >= srclen)
    {
        strcpy(dst, src);
    }
    else
    {
        strncpy(dst, src + srclen - nchars, nchars);
        dst[nchars] = 0;
    }

    return dst;
}
