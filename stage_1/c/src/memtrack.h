/* Counts the bytes allocated by our own code so the benchmarks can report the
   peak memory used while indexing (like tracemalloc in Python, which also
   ignores the internal memory of libraries such as SQLite).
   The Makefile includes this header in every file (-include), so every
   malloc/calloc/realloc/free of the project goes through the counters. */
#ifndef MEMTRACK_H
#define MEMTRACK_H

/* This header is included before anything else, so it also enables the
   POSIX functions used by util.c on Linux/macOS */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include <stddef.h>
#include <stdlib.h>

void *mt_malloc(size_t size);
void *mt_calloc(size_t count, size_t size);
void *mt_realloc(void *ptr, size_t size);
void mt_free(void *ptr);

/* Starts a new measurement / bytes of peak growth since the last reset */
void memtrack_reset_peak(void);
size_t memtrack_peak(void);

#define malloc(size) mt_malloc(size)
#define calloc(count, size) mt_calloc(count, size)
#define realloc(ptr, size) mt_realloc(ptr, size)
#define free(ptr) mt_free(ptr)

#endif