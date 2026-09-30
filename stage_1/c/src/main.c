#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "book.h"
#include "config.h"
#include "datalake.h"
#include "util.h"

static const int SAMPLE_IDS[] = {11, 84, 98, 1342, 1661};
#define N_SAMPLE (sizeof(SAMPLE_IDS) / sizeof(SAMPLE_IDS[0]))

static const char *check(int ok) { return ok ? "OK" : "FAIL"; }

/* Stores the sample in each layout, then reads every book back and checks
   that it is identical to the original. */
static int test_layout(DatalakeLayout layout, Book *books) {
    char root[PATH_SIZE];
    snprintf(root, sizeof(root), "%s/%s", DATALAKE_DIR, datalake_name(layout));
    remove_tree(root);

    Datalake lake;
    datalake_init(&lake, layout, root);

    time_t now = time(NULL);
    double start = now_seconds();
    for (size_t i = 0; i < N_SAMPLE; i++) {
        datalake_save(&lake, SAMPLE_IDS[i], &books[i], now);
    }
    double write_ms = 1000 * (now_seconds() - start);

    int same = 1;
    start = now_seconds();
    for (size_t i = 0; i < N_SAMPLE; i++) {
        Book stored;
        if (datalake_read(&lake, SAMPLE_IDS[i], &stored) != 0) { same = 0; continue; }
        same &= strcmp(stored.header, books[i].header) == 0 &&
                strcmp(stored.body, books[i].body) == 0;
        free_book(&stored);
    }
    double read_ms = 1000 * (now_seconds() - start);

    IdList ids;
    datalake_book_ids(&lake, &ids);
    int ids_ok = ids.count == N_SAMPLE;
    for (size_t i = 0; ids_ok && i < N_SAMPLE; i++) ids_ok = ids.ids[i] == SAMPLE_IDS[i];
    free_ids(&ids);

    StorageStats stats = datalake_stats(&lake);
    printf("  %-6s %8.1f %8.1f %6ld %6ld %10.2f   read %s, ids %s\n",
           datalake_name(layout), write_ms, read_ms, stats.files, stats.dirs,
           stats.bytes / 1e6, check(same), check(ids_ok));
    return same && ids_ok;
}

int main(void) {
    printf("Stage 1 - C implementation\n\n");

    Book books[N_SAMPLE];
    for (size_t i = 0; i < N_SAMPLE; i++) {
        char path[256];
        snprintf(path, sizeof(path), "%s/pg%d.txt", SAMPLE_DIR, SAMPLE_IDS[i]);
        char *text = read_file(path, NULL);
        if (!text || split_book(text, &books[i]) != 0) {
            printf("Could not read %s\n", path);
            return 1;
        }
        free(text);
    }

    printf("  %-6s %8s %8s %6s %6s %10s\n", "layout", "write ms", "read ms", "files", "dirs", "MB");
    int ok = 0;
    for (int layout = LAYOUT_TIME; layout <= LAYOUT_RANGE; layout++) {
        ok += test_layout((DatalakeLayout)layout, books);
    }
    printf("\n%d of 3 datalake layouts working correctly\n", ok);

    for (size_t i = 0; i < N_SAMPLE; i++) free_book(&books[i]);
    return ok == 3 ? 0 : 1;
}