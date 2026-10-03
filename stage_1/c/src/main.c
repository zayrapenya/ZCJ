#include <stdio.h>
#include <stdlib.h>

#include "book.h"
#include "config.h"
#include "disk_index.h"
#include "tokenizer.h"
#include "util.h"

static const int SAMPLE_IDS[] = {11, 84, 98, 1342, 1661};
#define N_SAMPLE (sizeof(SAMPLE_IDS) / sizeof(SAMPLE_IDS[0]))

/* Reference values from the Python implementation */
#define EXPECTED_DARCY 432
static const long EXPECTED_FILES[N_INDEX_KINDS] = {1, 16804, 1};
static const long long EXPECTED_BYTES[N_INDEX_KINDS] = {3347317, 3001886, 0}; /* 0 = not compared */

static const char *check(int ok) { return ok ? "OK" : "FAIL"; }

static size_t positions_in(const PostingList *list, int book_id) {
    for (size_t i = 0; i < list->count; i++) {
        if (list->items[i].book_id == book_id) return list->items[i].count;
    }
    return 0;
}

static int test_index(IndexKind kind, TokenList *books) {
    char datamarts[256];
    snprintf(datamarts, sizeof(datamarts), "%s/%s", DATAMARTS_DIR, index_kind_name(kind));
    remove_tree(datamarts);

    DiskIndex index;
    if (disk_index_open(&index, kind, datamarts) != 0) {
        printf("  %-13s could not open\n", index_kind_name(kind));
        return 0;
    }
    double start = now_seconds();
    for (size_t i = 0; i < N_SAMPLE; i++) {
        disk_index_add_book(&index, SAMPLE_IDS[i], &books[i]);
    }
    disk_index_flush(&index);
    double build_sec = now_seconds() - start;
    disk_index_close(&index);

    /* Reopen so the lookups read from disk */
    disk_index_open(&index, kind, datamarts);
    PostingList darcy, monster, missing;
    start = now_seconds();
    disk_index_lookup(&index, "darcy", &darcy);
    disk_index_lookup(&index, "monster", &monster);
    disk_index_lookup(&index, "zzzz", &missing);
    double lookup_ms = 1000 * (now_seconds() - start);

    int lookups_ok = positions_in(&darcy, 1342) == EXPECTED_DARCY && monster.count == 2 &&
                     positions_in(&monster, 84) > 0 && positions_in(&monster, 98) > 0 &&
                     missing.count == 0;
    DiskUsage usage = disk_index_usage(&index);
    int usage_ok = usage.files == EXPECTED_FILES[kind] &&
                   (EXPECTED_BYTES[kind] == 0 || usage.bytes == EXPECTED_BYTES[kind]);

    printf("  %-13s %8.3f %9.2f %7ld %10.2f   lookups %s, size %s\n", index_kind_name(kind),
           build_sec, lookup_ms, usage.files, usage.bytes / 1e6, check(lookups_ok), check(usage_ok));

    free_postings(&darcy);
    free_postings(&monster);
    free_postings(&missing);
    disk_index_close(&index);
    return lookups_ok && usage_ok;
}

int main(void) {
    printf("Stage 1 - C implementation\n\n");

    TokenList books[N_SAMPLE];
    for (size_t i = 0; i < N_SAMPLE; i++) {
        char path[256];
        snprintf(path, sizeof(path), "%s/pg%d.txt", SAMPLE_DIR, SAMPLE_IDS[i]);
        char *text = read_file(path, NULL);
        Book book;
        if (!text || split_book(text, &book) != 0) {
            printf("Could not read %s\n", path);
            return 1;
        }
        tokenize(book.body, &books[i]);
        free_book(&book);
        free(text);
    }

    printf("  %-13s %8s %9s %7s %10s\n", "index", "build s", "lookup ms", "files", "MB");
    int ok = 0;
    for (int kind = 0; kind < N_INDEX_KINDS; kind++) {
        ok += test_index((IndexKind)kind, books);
    }
    printf("\n%d of %d index structures match Python\n", ok, N_INDEX_KINDS);

    for (size_t i = 0; i < N_SAMPLE; i++) free_tokens(&books[i]);
    return ok == N_INDEX_KINDS ? 0 : 1;
}