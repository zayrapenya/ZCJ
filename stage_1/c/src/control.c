#include "control.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "book.h"
#include "config.h"
#include "tokenizer.h"
#include "util.h"

static int compare_ints(const void *a, const void *b) {
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}

int control_read(const char *path, IdList *out) {
    out->ids = NULL;
    out->count = 0;
    char *text = read_file(path, NULL);
    if (!text) return 0;

    size_t capacity = 64;
    out->ids = malloc(capacity * sizeof(int));
    if (!out->ids) { free(text); return -1; }

    for (char *line = strtok(text, "\r\n"); line; line = strtok(NULL, "\r\n")) {
        if (line[0] < '0' || line[0] > '9') continue;
        if (out->count == capacity) {
            capacity *= 2;
            int *bigger = realloc(out->ids, capacity * sizeof(int));
            if (!bigger) break;
            out->ids = bigger;
        }
        out->ids[out->count++] = atoi(line);
    }
    free(text);
    qsort(out->ids, out->count, sizeof(int), compare_ints);
    return 0;
}

int control_add(const char *path, int book_id) {
    char line[32];
    snprintf(line, sizeof(line), "%d\n", book_id);
    return append_file(path, line);
}

int id_list_contains(const IdList *list, int book_id) {
    return list->count > 0 &&
           bsearch(&book_id, list->ids, list->count, sizeof(int), compare_ints) != NULL;
}

void pipeline_init(Pipeline *p, Datalake *datalake, DiskIndex *index,
                   MetadataStore *metadata, const char *control_dir, FetchFn fetch) {
    p->datalake = datalake;
    p->index = index;
    p->metadata = metadata;
    p->fetch = fetch;
    make_dirs(control_dir);
    snprintf(p->downloaded, sizeof(p->downloaded), "%s/downloaded_books.txt", control_dir);
    snprintf(p->indexed, sizeof(p->indexed), "%s/indexed_books.txt", control_dir);
    snprintf(p->failed, sizeof(p->failed), "%s/failed_books.txt", control_dir);
}

static int in_control_file(const char *path, int book_id) {
    IdList ids;
    control_read(path, &ids);
    int found = id_list_contains(&ids, book_id);
    free_ids(&ids);
    return found;
}

int pipeline_download(Pipeline *p, int book_id) {
    if (in_control_file(p->downloaded, book_id)) {
        printf("[CONTROL] Book %d already downloaded, skipping.\n", book_id);
        return 1;
    }

    char *text = NULL;
    FetchStatus status = p->fetch(book_id, &text);
    if (status == FETCH_ERROR) {
        /* Network problem: not marked as failed, it will be retried */
        printf("[CONTROL] Book %d could not be downloaded, will retry later.\n", book_id);
        return 0;
    }

    Book book;
    if (status == FETCH_NOT_FOUND || split_book(text, &book) != 0) {
        printf("[CONTROL] Book %d discarded (%s).\n", book_id,
               status == FETCH_NOT_FOUND ? "book not found" : "Gutenberg markers not found");
        control_add(p->failed, book_id);
        free(text);
        return 0;
    }
    free(text);

    int rc = datalake_save(p->datalake, book_id, &book, time(NULL));
    free_book(&book);
    if (rc != 0) return 0;

    control_add(p->downloaded, book_id);
    printf("[CONTROL] Book %d successfully downloaded.\n", book_id);
    return 1;
}

int pipeline_index_book(Pipeline *p, int book_id) {
    Book book;
    char header_path[PATH_SIZE], body_path[PATH_SIZE];
    if (datalake_read(p->datalake, book_id, &book) != 0 ||
        datalake_locate(p->datalake, book_id, header_path, body_path) != 0) {
        printf("[CONTROL] Book %d not found in the datalake.\n", book_id);
        return 0;
    }

    Metadata meta;
    parse_header(book.header, &meta);
    metadata_insert(p->metadata, book_id, &meta, body_path, 1);
    free_metadata(&meta);

    TokenList tokens;
    tokenize(book.body, &tokens);
    int rc = disk_index_add_book(p->index, book_id, &tokens);
    if (rc == 0) rc = disk_index_flush(p->index);
    free_tokens(&tokens);
    free_book(&book);
    if (rc != 0) return 0;

    control_add(p->indexed, book_id);
    printf("[CONTROL] Book %d successfully indexed.\n", book_id);
    return 1;
}

int pipeline_pending(Pipeline *p, IdList *out) {
    IdList downloaded, indexed;
    control_read(p->downloaded, &downloaded);
    control_read(p->indexed, &indexed);

    out->ids = malloc((downloaded.count ? downloaded.count : 1) * sizeof(int));
    out->count = 0;
    for (size_t i = 0; out->ids && i < downloaded.count; i++) {
        int id = downloaded.ids[i];
        int duplicate = out->count > 0 && out->ids[out->count - 1] == id;
        if (!duplicate && !id_list_contains(&indexed, id)) out->ids[out->count++] = id;
    }
    free_ids(&downloaded);
    free_ids(&indexed);
    return out->ids ? 0 : -1;
}

void pipeline_step(Pipeline *p, const int *candidates, size_t n_candidates) {
    IdList pending;
    pipeline_pending(p, &pending);
    if (pending.count > 0) {
        int book_id = pending.ids[0];  /* smallest id, like min() in Python */
        free_ids(&pending);
        printf("[CONTROL] Scheduling book %d for indexing...\n", book_id);
        pipeline_index_book(p, book_id);
        return;
    }
    free_ids(&pending);

    IdList downloaded, failed;
    control_read(p->downloaded, &downloaded);
    control_read(p->failed, &failed);

    int random_ids[10];
    if (!candidates || n_candidates == 0) {  /* retry up to 10 random ids */
        static int seeded = 0;
        if (!seeded) { srand((unsigned)time(NULL)); seeded = 1; }
        for (int i = 0; i < 10; i++) {
            /* rand() can be as small as 32767 on Windows, so two calls are combined */
            long r = ((long)rand() << 15) ^ rand();
            random_ids[i] = (int)(r % TOTAL_BOOKS) + 1;
        }
        candidates = random_ids;
        n_candidates = 10;
    }

    int done = 0;
    for (size_t i = 0; i < n_candidates && !done; i++) {
        int book_id = candidates[i];
        if (id_list_contains(&downloaded, book_id) || id_list_contains(&failed, book_id)) continue;
        printf("[CONTROL] Downloading new book with ID %d...\n", book_id);
        done = pipeline_download(p, book_id);
    }
    if (!done) printf("[CONTROL] No new books to download.\n");

    free_ids(&downloaded);
    free_ids(&failed);
}

void pipeline_run(Pipeline *p, int steps, const int *candidates, size_t n_candidates) {
    for (int i = 0; i < steps; i++) pipeline_step(p, candidates, n_candidates);
}