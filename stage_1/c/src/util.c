#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "util.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#define MKDIR(p) _mkdir(p)
#else
#include <time.h>
#define MKDIR(p) mkdir(p, 0755)
#endif

/* Files are always opened in binary mode so Windows does not convert
   line endings and output matches the Python and Java versions. */

char *read_file(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size < 0) { fclose(f); return NULL; }

    char *buffer = malloc((size_t)size + 1);
    if (!buffer) { fclose(f); return NULL; }

    size_t read = fread(buffer, 1, (size_t)size, f);
    fclose(f);
    buffer[read] = '\0';
    if (out_len) *out_len = read;
    return buffer;
}

int write_file(const char *path, const char *data, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    size_t written = fwrite(data, 1, len, f);
    fclose(f);
    return written == len ? 0 : -1;
}

int append_file(const char *path, const char *text) {
    FILE *f = fopen(path, "ab");
    if (!f) return -1;
    fputs(text, f);
    fclose(f);
    return 0;
}

int make_dirs(const char *path) {
    char buffer[1024];
    size_t len = strlen(path);
    if (len == 0 || len >= sizeof(buffer)) return -1;
    memcpy(buffer, path, len + 1);

    for (size_t i = 1; i <= len; i++) {
        if (buffer[i] == '/' || buffer[i] == '\\' || buffer[i] == '\0') {
            char saved = buffer[i];
            buffer[i] = '\0';
            if (MKDIR(buffer) != 0 && errno != EEXIST) return -1;
            buffer[i] = saved;
        }
    }
    return 0;
}

int path_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

double now_seconds(void) {
#ifdef _WIN32
    static LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
    if (frequency.QuadPart == 0) QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)frequency.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
#endif
}