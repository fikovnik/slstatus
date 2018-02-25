/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../util.h"

const char *
text_file(const char *path)
{
	char s[16];

    if(access(path, F_OK ) != -1) {
        return (pscanf(path, "%s", &s[0]) == 1) ?
            bprintf("%s", s) : NULL;
    }

    return NULL;
}
