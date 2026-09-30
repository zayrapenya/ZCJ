#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

/* Returned buffer is NUL-terminated and must be freed by the caller. */
char *read_file(const char *path, size_t *out_len);
int write_file(const char *path, const char *data, size_t len);
int append_file(const char *path, const char *text);
int make_dirs(const char *path);
int path_exists(const char *path);
double now_seconds(void);

#endif