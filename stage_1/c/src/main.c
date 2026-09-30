#include <stdio.h>
#include <stdlib.h>

#include "book.h"
#include "config.h"
#include "index.h"
#include "tokenizer.h"
#include "util.h"

static const int SAMPLE_IDS[] = {11, 84, 98, 1342, 1661};
#define N_SAMPLE (sizeof(SAMPLE_IDS) / sizeof(SAMPLE_IDS[0]))

/* Reference values from the Python implementation */
static const size_t EXPECTED_TOKENS[] = {27427, 75328, 138490, 128565, 105863};
static const long EXPECTED_DISTINCT[] = {2575, 6977, 9698, 6729, 7817};
#define EXPECTED_TERMS 16804
#define EXPECTED_DARCY 432

static const char *check(int ok) { return ok ? "OK" : "DISTINTO"; }

int main(void) {
    printf("Stage 1 - implementacion en C\n\n");
    printf("  %-6s %10s %10s  %s\n", "libro", "palabras", "distintas", "");

    InvertedIndex *index = index_create();
    int ok = 0, total_checks = 0;
    double start = now_seconds();

    for (size_t i = 0; i < N_SAMPLE; i++) {
        char path[256];
        snprintf(path, sizeof(path), "%s/pg%d.txt", SAMPLE_DIR, SAMPLE_IDS[i]);

        char *text = read_file(path, NULL);
        Book book;
        if (!text || split_book(text, &book) != 0) {
            printf("  %-6d NO SE PUDO LEER (%s)\n", SAMPLE_IDS[i], path);
            free(text);
            continue;
        }

        TokenList tokens;
        tokenize(book.body, &tokens);
        long distinct = index_add_book(index, SAMPLE_IDS[i], &tokens);

        int same = tokens.count == EXPECTED_TOKENS[i] && distinct == EXPECTED_DISTINCT[i];
        ok += same;
        total_checks++;
        printf("  %-6d %10zu %10ld  %s\n", SAMPLE_IDS[i], tokens.count, distinct, check(same));

        free_tokens(&tokens);
        free_book(&book);
        free(text);
    }
    double elapsed = now_seconds() - start;

    size_t terms = index_term_count(index);
    const Posting *darcy = postings_find(index_lookup(index, "darcy"), 1342);
    size_t darcy_count = darcy ? darcy->count : 0;

    printf("\n  palabras distintas en el indice: %zu (python: %d)  %s\n",
           terms, EXPECTED_TERMS, check(terms == EXPECTED_TERMS));
    printf("  \"darcy\" en el libro 1342: %zu veces (python: %d)  %s\n",
           darcy_count, EXPECTED_DARCY, check(darcy_count == EXPECTED_DARCY));
    ok += (terms == EXPECTED_TERMS) + (darcy_count == EXPECTED_DARCY);
    total_checks += 2;

    printf("\n%d de %d comprobaciones coinciden con Python (indice construido en %.1f ms)\n",
           ok, total_checks, 1000 * elapsed);

    index_free(index);
    return ok == total_checks ? 0 : 1;
}