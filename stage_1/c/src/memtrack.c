#include "memtrack.h"

#include <string.h>

/* The real allocator is used here */
#undef malloc
#undef calloc
#undef realloc
#undef free

/* Every block starts with a header that stores its size */
typedef union {
    size_t size;
    max_align_t align;
} Header;

static size_t in_use = 0;
static size_t peak = 0;
static size_t baseline = 0;

static void count(size_t size) {
    in_use += size;
    if (in_use > peak) peak = in_use;
}

void *mt_malloc(size_t size) {
    Header *header = malloc(sizeof(Header) + size);
    if (!header) return NULL;
    header->size = size;
    count(size);
    return header + 1;
}

void *mt_calloc(size_t n, size_t size) {
    void *ptr = mt_malloc(n * size);
    if (ptr) memset(ptr, 0, n * size);
    return ptr;
}

void *mt_realloc(void *ptr, size_t size) {
    if (!ptr) return mt_malloc(size);
    Header *header = (Header *)ptr - 1;
    size_t old_size = header->size;
    Header *moved = realloc(header, sizeof(Header) + size);
    if (!moved) return NULL;
    in_use -= old_size;
    moved->size = size;
    count(size);
    return moved + 1;
}

void mt_free(void *ptr) {
    if (!ptr) return;
    Header *header = (Header *)ptr - 1;
    in_use -= header->size;
    free(header);
}

void memtrack_reset_peak(void) {
    baseline = peak = in_use;
}

size_t memtrack_peak(void) {
    return peak - baseline;
}