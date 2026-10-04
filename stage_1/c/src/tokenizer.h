#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stddef.h>

typedef struct {
    char *buffer;
    char **words;
    size_t count;
} TokenList;

/* Lowercase, split on anything that is not a-z (same rules as Python/Java). */
int tokenize(const char *text, TokenList *out);
void free_tokens(TokenList *tokens);

#endif