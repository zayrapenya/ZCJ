#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "book.h"
#include "config.h"
#include "metadata.h"
#include "util.h"

static const int SAMPLE_IDS[] = {11, 84, 98, 1342, 1661};
#define N_SAMPLE (sizeof(SAMPLE_IDS) / sizeof(SAMPLE_IDS[0]))

/* Reference values from the Python implementation */
static const char *EXPECTED[N_SAMPLE][4] = {
    {"Alice's Adventures in Wonderland", "Lewis Carroll", "June 27, 2008", "English"},
    {"Frankenstein; or, the modern prometheus", "Mary Wollstonecraft Shelley", "October 1, 1993", "English"},
    {"A Tale of Two Cities", "Charles Dickens", "January 1, 1994", "English"},
    {"Pride and Prejudice", "Jane Austen", "June 1, 1998", "English"},
    {"The Adventures of Sherlock Holmes", "Arthur Conan Doyle", "March 1, 1999", "English"},
};

static int same(const char *a, const char *b) {
    return a && b && strcmp(a, b) == 0;
}

static const char *check(int ok) { return ok ? "OK" : "FAIL"; }

int main(void) {
    printf("Stage 1 - C implementation\n\n");

    char db_path[256];
    snprintf(db_path, sizeof(db_path), "%s/metadata.db", DATAMARTS_DIR);
    remove(db_path);

    MetadataStore store;
    if (metadata_open(&store, db_path) != 0) {
        printf("Could not open %s\n", db_path);
        return 1;
    }

    int ok = 0, checks = 0;
    for (size_t i = 0; i < N_SAMPLE; i++) {
        char path[256];
        snprintf(path, sizeof(path), "%s/pg%d.txt", SAMPLE_DIR, SAMPLE_IDS[i]);
        char *text = read_file(path, NULL);
        Book book;
        if (!text || split_book(text, &book) != 0) {
            printf("Could not read %s\n", path);
            free(text);
            continue;
        }

        Metadata meta;
        parse_header(book.header, &meta);
        int match = same(meta.title, EXPECTED[i][0]) && same(meta.author, EXPECTED[i][1]) &&
                    same(meta.release_date, EXPECTED[i][2]) && same(meta.language, EXPECTED[i][3]);
        ok += match;
        checks++;
        printf("  %-5d %-40.40s %-28s %-16s %s\n", SAMPLE_IDS[i], meta.title, meta.author,
               meta.release_date, check(match));

        char book_path[256];
        snprintf(book_path, sizeof(book_path), "%s/books/%d/body.txt", DATALAKE_DIR, SAMPLE_IDS[i]);
        metadata_insert(&store, SAMPLE_IDS[i], &meta, book_path, 1);

        free_metadata(&meta);
        free_book(&book);
        free(text);
    }

    long count = metadata_count(&store);
    printf("\n  rows in table: %ld  %s\n", count, check(count == (long)N_SAMPLE));
    ok += count == (long)N_SAMPLE;
    checks++;

    BookRows rows;
    metadata_by_author(&store, "Austen", &rows);
    int author_ok = rows.count == 1 && rows.rows[0].book_id == 1342;
    printf("  by author \"Austen\": %zu book(s)  %s\n", rows.count, check(author_ok));
    free_book_rows(&rows);
    ok += author_ok;
    checks++;

    char *path = metadata_path_by_title(&store, "Pride and Prejudice");
    int path_ok = same(path, "datalake/books/1342/body.txt");
    printf("  path of \"Pride and Prejudice\": %s  %s\n", path ? path : "(none)", check(path_ok));
    free(path);
    ok += path_ok;
    checks++;

    metadata_by_language(&store, "English", &rows);
    printf("  by language \"English\": %zu book(s)  %s\n", rows.count, check(rows.count == N_SAMPLE));
    ok += rows.count == N_SAMPLE;
    checks++;
    free_book_rows(&rows);

    metadata_close(&store);
    printf("\n%d of %d checks match Python\n", ok, checks);
    return ok == checks ? 0 : 1;
}