#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/uio.h>
#include <dlfcn.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>

#include "execinfo.h"
#include "stacktraverse.h"

#define D10(x) ceil(log10(((x) == 0) ? 2 : ((x) + 1)))
#define MAX_STACK_BUFFER 4096

inline static void * __attribute__((unused))
realloc_safe(void *ptr, size_t size)
{
    void *nptr;
    if (size == 0 || size > SIZE_MAX / 2) {
        free(ptr);
        return NULL;
    }
    nptr = realloc(ptr, size);
    if (nptr == NULL) {
        free(ptr);
        errno = ENOMEM;
    }
    return nptr;
}

int
backtrace(void **buffer, int size)
{
    int i;
    if (size <= 0)
        return 0;

    for (i = 0; i < size; i++) {
        void *addr = getreturnaddr(i);
        if (addr == NULL)
            break;
        buffer[i] = addr;
    }
    return i;
}

char **
backtrace_symbols(void *const *buffer, int size)
{
    if (size <= 0)
        return NULL;

    size_t total_len = 0;
    char temp[512];
    Dl_info info;
    ptrdiff_t offset = 0;
    int i;
    for (i = 0; i < size; i++) {
        if (!buffer[i]) {
            snprintf(temp, sizeof(temp), "%p", buffer[i]);
        } else if (dladdr(buffer[i], &info) != 0) {
            if (!info.dli_sname) info.dli_sname = "???";
            if (!info.dli_saddr) info.dli_saddr = buffer[i];
            offset = (char *)buffer[i] - (char *)info.dli_saddr;
            snprintf(temp, sizeof(temp), "%p <%s+%td> at %s",
                     buffer[i], info.dli_sname, offset, info.dli_fname);
        } else {
            snprintf(temp, sizeof(temp), "%p", buffer[i]);
        }
        total_len += strlen(temp) + 1; // for '\0'
    }

    size_t ptrs_size = size * sizeof(char *);
    char **rval = malloc(ptrs_size + total_len);
    if (!rval) return NULL;

    char *strings = (char *)(rval + size);
    char *cur = strings;
    for (i = 0; i < size; i++) {
        if (!buffer[i]) {
            snprintf(temp, sizeof(temp), "%p", buffer[i]);
        } else if (dladdr(buffer[i], &info) != 0) {
            if (!info.dli_sname) info.dli_sname = "???";
            if (!info.dli_saddr) info.dli_saddr = buffer[i];
            offset = (char *)buffer[i] - (char *)info.dli_saddr;
            snprintf(temp, sizeof(temp), "%p <%s+%td> at %s",
                     buffer[i], info.dli_sname, offset, info.dli_fname);
        } else {
            snprintf(temp, sizeof(temp), "%p", buffer[i]);
        }
        size_t len = strlen(temp) + 1;
        memcpy(cur, temp, len);
        rval[i] = cur;
        cur += len;
    }
    return rval;
}

void
backtrace_symbols_fd(void *const *buffer, int size, int fd)
{
    int i, len;
    char *buf;
    char static_buf[MAX_STACK_BUFFER];
    Dl_info info;
    ptrdiff_t offset;
    ssize_t written;

    if (size <= 0 || fd < 0)
        return;

    for (i = 0; i < size; i++) {
        if (!buffer[i]) {
            len = 2 + (sizeof(void *) * 2) + 2;
            if (len <= MAX_STACK_BUFFER) {
                buf = static_buf;
            } else {
                buf = malloc(len);
                if (buf == NULL)
                    return;
            }
            snprintf(buf, len, "%p\n", buffer[i]);
        } else if (dladdr(buffer[i], &info) != 0) {
            if (info.dli_sname == NULL)
                info.dli_sname = "???";
            if (info.dli_saddr == NULL)
                info.dli_saddr = buffer[i];
            offset = (char *)buffer[i] - (char *)info.dli_saddr;
            len = 2 + (sizeof(void *) * 2) + 2 + strlen(info.dli_sname) + 1 +
                  D10(offset) + 5 + strlen(info.dli_fname) + 2;
            if (len <= MAX_STACK_BUFFER) {
                buf = static_buf;
            } else {
                buf = malloc(len);
                if (buf == NULL)
                    return;
            }
            snprintf(buf, len, "%p <%s+%td> at %s\n",
                buffer[i], info.dli_sname, offset, info.dli_fname);
        } else {
            len = 2 + (sizeof(void *) * 2) + 2;
            if (len <= MAX_STACK_BUFFER) {
                buf = static_buf;
            } else {
                buf = malloc(len);
                if (buf == NULL)
                    return;
            }
            snprintf(buf, len, "%p\n", buffer[i]);
        }

        written = write(fd, buf, strlen(buf));
        (void)written;
        if (buf != static_buf)
            free(buf);
    }
}
