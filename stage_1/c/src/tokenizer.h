/* Tokenizador: mismas reglas que Python y Java.
   Pasar a minúsculas, todo lo que no sea a-z es separador, y quedarse con las palabras. */
#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stddef.h>

typedef struct {
    char *buffer;    /* copia del texto donde viven todas las palabras */
    char **words;    /* words[i] es la palabra en la posición i */
    size_t count;    /* número de palabras */
} TokenList;

/* Trocea el texto en palabras. Devuelve 0 si va bien.
   Hay que liberar el resultado con free_tokens(). */
int tokenize(const char *text, TokenList *out);

void free_tokens(TokenList *tokens);

#endif