#include "tokenizer.h"

#include <stdlib.h>
#include <string.h>

/* Words are stored in a single copy of the text where every separator is
   replaced by '\0', so each word is a string pointing into that buffer.
   Non-ASCII bytes fall outside a-z and act as separators, like in Python. */
int tokenize(const char *text, TokenList *out) {
    size_t len = strlen(text);
    out->buffer = malloc(len + 1);
    out->words = NULL;
    out->count = 0;
    if (!out->buffer) return -1;

    size_t capacity = 1024;
    out->words = malloc(capacity * sizeof(char *));
    if (!out->words) { free(out->buffer); return -1; }

    int inside_word = 0;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)text[i];
        if (c >= 'A' && c <= 'Z') c = (unsigned char)(c - 'A' + 'a');

        if (c >= 'a' && c <= 'z') {
            out->buffer[i] = (char)c;
            if (!inside_word) {
                if (out->count == capacity) {
                    capacity *= 2;
                    char **bigger = realloc(out->words, capacity * sizeof(char *));
                    if (!bigger) { free_tokens(out); return -1; }
                    out->words = bigger;
                }
                out->words[out->count++] = &out->buffer[i];
                inside_word = 1;
            }
        } else {
            out->buffer[i] = '\0';
            inside_word = 0;
        }
    }
    out->buffer[len] = '\0';
    return 0;
}

void free_tokens(TokenList *tokens) {
    free(tokens->buffer);
    free(tokens->words);
    tokens->buffer = NULL;
    tokens->words = NULL;
    tokens->count = 0;
}