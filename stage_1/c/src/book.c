#include "book.h"

#include <stdlib.h>
#include <string.h>

#include "config.h"

static int is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

static char *copy_stripped(const char *text, size_t start, size_t end) {
    while (start < end && is_space(text[start])) start++;
    while (end > start && is_space(text[end - 1])) end--;

    size_t len = end - start;
    char *copy = malloc(len + 1);
    if (!copy) return NULL;
    memcpy(copy, text + start, len);
    copy[len] = '\0';
    return copy;
}

int split_book(const char *text, Book *out) {
    const char *start = strstr(text, START_MARKER);
    const char *end = strstr(text, END_MARKER);
    if (!start || !end) return -1;

    size_t start_pos = (size_t)(start - text);
    size_t end_pos = (size_t)(end - text);

    /* The body starts after the whole marker line, as in the Python version. */
    const char *newline = strchr(start, '\n');
    size_t body_start = newline ? (size_t)(newline - text) : start_pos;
    if (body_start > end_pos) return -1;

    out->header = copy_stripped(text, 0, start_pos);
    out->body = copy_stripped(text, body_start, end_pos);
    if (!out->header || !out->body) {
        free_book(out);
        return -1;
    }
    return 0;
}

void free_book(Book *book) {
    free(book->header);
    free(book->body);
    book->header = book->body = NULL;
}