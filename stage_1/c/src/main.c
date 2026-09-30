/* Punto de entrada. Paso 1: separar header/body y tokenizar el sample data,
   comprobando que salen las mismas palabras que en Python. */

#include <stdio.h>
#include <stdlib.h>

#include "book.h"
#include "config.h"
#include "tokenizer.h"
#include "util.h"

static const int SAMPLE_IDS[] = {11, 84, 98, 1342, 1661};
/* Número de palabras que obtiene la versión de Python para cada libro */
static const size_t EXPECTED_TOKENS[] = {27427, 75328, 138490, 128565, 105863};
#define N_SAMPLE (sizeof(SAMPLE_IDS) / sizeof(SAMPLE_IDS[0]))

int main(void) {
    printf("Stage 1 - implementacion en C\n\n");
    printf("  %-6s %10s %10s  %s\n", "libro", "palabras", "python", "");

    int ok = 0;
    double start = now_seconds();

    for (size_t i = 0; i < N_SAMPLE; i++) {
        char path[256];
        snprintf(path, sizeof(path), "%s/pg%d.txt", SAMPLE_DIR, SAMPLE_IDS[i]);

        char *text = read_file(path, NULL);
        if (!text) {
            printf("  %-6d NO ENCONTRADO (%s)\n", SAMPLE_IDS[i], path);
            continue;
        }

        Book book;
        if (split_book(text, &book) != 0) {
            printf("  %-6d sin marcadores de Gutenberg\n", SAMPLE_IDS[i]);
            free(text);
            continue;
        }

        TokenList tokens;
        tokenize(book.body, &tokens);

        int same = tokens.count == EXPECTED_TOKENS[i];
        ok += same;
        printf("  %-6d %10zu %10zu  %s\n", SAMPLE_IDS[i], tokens.count,
               EXPECTED_TOKENS[i], same ? "OK" : "DISTINTO");

        free_tokens(&tokens);
        free_book(&book);
        free(text);
    }

    printf("\n%d de %zu libros coinciden con Python (%.1f ms)\n",
           ok, N_SAMPLE, 1000 * (now_seconds() - start));
    return ok == (int)N_SAMPLE ? 0 : 1;
}