#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "book.h"
#include "config.h"
#include "disk_index.h"
#include "search.h"
#include "tokenizer.h"
#include "util.h"

static const int SAMPLE_IDS[] = {11, 84, 98, 1342, 1661};
#define N_SAMPLE (sizeof(SAMPLE_IDS) / sizeof(SAMPLE_IDS[0]))

typedef enum { WORD, AND, PHRASE } QueryType;

typedef struct {
    QueryType type;
    const char *text;       /* AND queries: words separated by spaces */
    size_t books;           /* expected results, from the Python version */
    size_t hits;
} Query;

/* Same query workload as the benchmarks of every language */
static const Query QUERIES[] = {
    {WORD, "darcy", 1, 432},       {WORD, "monster", 2, 32},
    {WORD, "alice", 2, 411},       {WORD, "holmes", 1, 462},
    {WORD, "love", 5, 238},        {WORD, "the", 5, 24192},
    {WORD, "revolution", 3, 12},   {WORD, "creature", 5, 98},
    {WORD, "queen", 4, 90},        {WORD, "zzzz", 0, 0},
    {AND, "darcy love", 1, 432},   {AND, "monster night", 2, 32},
    {AND, "alice queen", 2, 411},  {AND, "holmes watson", 1, 462},
    {PHRASE, "mr darcy", 1, 277},  {PHRASE, "sherlock holmes", 1, 97},
    {PHRASE, "the white rabbit", 1, 21},
    {PHRASE, "it was the best of times", 1, 1},
};
#define N_QUERIES (sizeof(QUERIES) / sizeof(QUERIES[0]))

static const char *TYPE_NAMES[] = {"word", "and", "phrase"};

/* Builds the index of the sample in datamarts/<kind> if it does not exist yet */
static int open_sample_index(DiskIndex *index, IndexKind kind) {
    char datamarts[256];
    snprintf(datamarts, sizeof(datamarts), "%s/%s", DATAMARTS_DIR, index_kind_name(kind));
    if (disk_index_open(index, kind, datamarts) != 0) return -1;

    PostingList probe;
    disk_index_lookup(index, "darcy", &probe);
    int exists = probe.count > 0;
    free_postings(&probe);
    if (exists) return 0;

    printf("  building %s index...\n", index_kind_name(kind));
    for (size_t i = 0; i < N_SAMPLE; i++) {
        char path[256];
        snprintf(path, sizeof(path), "%s/pg%d.txt", SAMPLE_DIR, SAMPLE_IDS[i]);
        char *text = read_file(path, NULL);
        Book book;
        if (!text || split_book(text, &book) != 0) { free(text); return -1; }
        TokenList tokens;
        tokenize(book.body, &tokens);
        disk_index_add_book(index, SAMPLE_IDS[i], &tokens);
        free_tokens(&tokens);
        free_book(&book);
        free(text);
    }
    return disk_index_flush(index);
}

static void run_query(DiskIndex *index, const Query *query, PostingList *out) {
    if (query->type == WORD) {
        search_word(index, query->text, out);
    } else if (query->type == PHRASE) {
        search_phrase(index, query->text, out);
    } else {
        char copy[256];
        const char *words[16];
        size_t n = 0;
        snprintf(copy, sizeof(copy), "%s", query->text);
        for (char *w = strtok(copy, " "); w && n < 16; w = strtok(NULL, " ")) words[n++] = w;
        search_and(index, words, n, out);
    }
}

int main(void) {
    printf("Stage 1 - C implementation\n\n");

    int total_ok = 0;
    for (int kind = 0; kind < N_INDEX_KINDS; kind++) {
        DiskIndex index;
        if (open_sample_index(&index, (IndexKind)kind) != 0) {
            printf("  %s: could not open the index\n", index_kind_name((IndexKind)kind));
            continue;
        }

        int ok = 0;
        double start = now_seconds();
        for (size_t q = 0; q < N_QUERIES; q++) {
            PostingList result;
            run_query(&index, &QUERIES[q], &result);

            size_t hits = 0;
            for (size_t b = 0; b < result.count; b++) hits += result.items[b].count;
            int same = result.count == QUERIES[q].books && hits == QUERIES[q].hits;
            ok += same;
            if (!same) {
                printf("  %s: %s \"%s\" -> %zu books, %zu hits (expected %zu, %zu)\n",
                       index_kind_name((IndexKind)kind), TYPE_NAMES[QUERIES[q].type],
                       QUERIES[q].text, result.count, hits, QUERIES[q].books, QUERIES[q].hits);
            }
            free_postings(&result);
        }
        double elapsed_ms = 1000 * (now_seconds() - start);

        printf("  %-13s %2d of %zu queries match Python  (%.2f ms)\n",
               index_kind_name((IndexKind)kind), ok, N_QUERIES, elapsed_ms);
        total_ok += ok == (int)N_QUERIES;
        disk_index_close(&index);
    }

    printf("\n%d of %d index structures answer every query like Python\n", total_ok, N_INDEX_KINDS);
    return total_ok == N_INDEX_KINDS ? 0 : 1;
}