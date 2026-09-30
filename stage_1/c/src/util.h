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

/* Calls visit() for every file and directory below root (root itself excluded).
   Stops and returns the first non-zero value returned by visit. */
typedef int (*walk_fn)(const char *path, const char *name, int is_dir, long long size, void *ctx);
int walk_dir(const char *root, walk_fn visit, void *ctx);

/* Deletes a directory and everything inside it (like "rm -rf"). */
int remove_tree(const char *path);

#endif