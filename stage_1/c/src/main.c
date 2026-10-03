/* Command line entry point, with the same commands as the Python version:
     search_engine [--datalake time|book|range] [--index monolithic|hierarchical|sqlite] <command>
       download 1342 84 1661       download books into the datalake
       index                       index every downloaded book not yet indexed
       run [--steps N] [--ids ...] control layer: alternate download / index
       sample                      ingest the offline sample dataset
       search "mr darcy" [--phrase]
       metadata [--author X | --language X | --id N] */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "control.h"
#include "datalake.h"
#include "disk_index.h"
#include "downloader.h"
#include "metadata.h"
#include "search.h"
#include "util.h"

#define MAX_IDS 1024

static void usage(void) {
    printf("Usage: search_engine [--datalake time|book|range] "
           "[--index monolithic|hierarchical|sqlite] <command>\n"
           "  download <ids...>\n"
           "  index\n"
           "  run [--steps N] [--ids <ids...>]\n"
           "  sample\n"
           "  search \"<query>\" [--phrase]\n"
           "  metadata [--author X | --language X | --id N]\n");
}

static int collect_sample_id(const char *path, const char *name, int is_dir,
                             long long size, void *ctx) {
    (void)path; (void)size;
    IdList *ids = ctx;
    int id;
    char ext[8];
    if (!is_dir && ids->count < MAX_IDS && sscanf(name, "pg%d.%4s", &id, ext) == 2 &&
        strcmp(ext, "txt") == 0) {
        ids->ids[ids->count++] = id;
    }
    return 0;
}

static int compare_ints(const void *a, const void *b) {
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}

static int compare_hits(const void *a, const void *b) {
    size_t x = ((const Posting *)a)->count, y = ((const Posting *)b)->count;
    return (x < y) - (x > y);   /* most hits first */
}

static void print_row(const BookRow *row) {
    printf("%6d | %s | %s | %s | %s | %s\n", row->book_id,
           row->meta.title ? row->meta.title : "-", row->meta.author ? row->meta.author : "-",
           row->meta.release_date ? row->meta.release_date : "-",
           row->meta.language ? row->meta.language : "-", row->path ? row->path : "-");
}

static void cmd_search(DiskIndex *index, MetadataStore *metadata, char *query, int phrase) {
    PostingList results;
    if (phrase) {
        search_phrase(index, query, &results);
    } else {
        const char *words[64];
        size_t n = 0;
        for (char *w = strtok(query, " "); w && n < 64; w = strtok(NULL, " ")) words[n++] = w;
        search_and(index, words, n, &results);
    }

    if (results.count == 0) printf("No results.\n");
    qsort(results.items, results.count, sizeof(Posting), compare_hits);
    for (size_t i = 0; i < results.count; i++) {
        BookRow row = {0};
        int found = metadata_get(metadata, results.items[i].book_id, &row) == 0;
        printf("%6d  %5zu hits  %s - %s\n", results.items[i].book_id, results.items[i].count,
               found && row.meta.title ? row.meta.title : "-",
               found && row.meta.author ? row.meta.author : "-");
        if (found) free_book_row(&row);
    }
    free_postings(&results);
}

static void cmd_metadata(MetadataStore *metadata, const char *author, const char *language, int id) {
    BookRows rows = {NULL, 0};
    if (id > 0) {
        BookRow row;
        if (metadata_get(metadata, id, &row) == 0) {
            print_row(&row);
            free_book_row(&row);
        }
        return;
    }
    if (author) {
        metadata_by_author(metadata, author, &rows);
    } else if (language) {
        metadata_by_language(metadata, language, &rows);
    } else {
        /* Every book, ordered by id */
        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(metadata->db, "SELECT book_id FROM books ORDER BY book_id", -1, &stmt, NULL);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            BookRow row;
            if (metadata_get(metadata, sqlite3_column_int(stmt, 0), &row) == 0) {
                print_row(&row);
                free_book_row(&row);
            }
        }
        sqlite3_finalize(stmt);
        return;
    }
    for (size_t i = 0; i < rows.count; i++) print_row(&rows.rows[i]);
    free_book_rows(&rows);
}

int main(int argc, char **argv) {
    const char *layout_name = "time";
    const char *index_name = "monolithic";

    int arg = 1;
    while (arg + 1 < argc && strncmp(argv[arg], "--", 2) == 0) {
        if (strcmp(argv[arg], "--datalake") == 0) layout_name = argv[arg + 1];
        else if (strcmp(argv[arg], "--index") == 0) index_name = argv[arg + 1];
        arg += 2;
    }
    if (arg >= argc) {
        usage();
        return 1;
    }
    const char *command = argv[arg++];

    DatalakeLayout layout;
    IndexKind kind;
    if (datalake_parse_layout(layout_name, &layout) != 0 || index_kind_parse(index_name, &kind) != 0) {
        usage();
        return 1;
    }

    Datalake datalake;
    datalake_init(&datalake, layout, DATALAKE_DIR);
    DiskIndex index;
    MetadataStore metadata;
    char metadata_path[256];
    snprintf(metadata_path, sizeof(metadata_path), "%s/metadata.db", DATAMARTS_DIR);
    if (disk_index_open(&index, kind, DATAMARTS_DIR) != 0 ||
        metadata_open(&metadata, metadata_path) != 0) {
        printf("Could not open the datamarts in %s\n", DATAMARTS_DIR);
        return 1;
    }

    Pipeline pipeline;
    int ids[MAX_IDS];
    size_t n_ids = 0;

    if (strcmp(command, "download") == 0) {
        pipeline_init(&pipeline, &datalake, &index, &metadata, CONTROL_DIR, download_book);
        for (; arg < argc; arg++) pipeline_download(&pipeline, atoi(argv[arg]));

    } else if (strcmp(command, "index") == 0) {
        pipeline_init(&pipeline, &datalake, &index, &metadata, CONTROL_DIR, download_book);
        IdList pending;
        pipeline_pending(&pipeline, &pending);
        for (size_t i = 0; i < pending.count; i++) pipeline_index_book(&pipeline, pending.ids[i]);
        free_ids(&pending);

    } else if (strcmp(command, "run") == 0) {
        int steps = 10;
        for (; arg < argc; arg++) {
            if (strcmp(argv[arg], "--steps") == 0 && arg + 1 < argc) {
                steps = atoi(argv[++arg]);
            } else if (strcmp(argv[arg], "--ids") != 0 && n_ids < MAX_IDS) {
                ids[n_ids++] = atoi(argv[arg]);
            }
        }
        pipeline_init(&pipeline, &datalake, &index, &metadata, CONTROL_DIR, download_book);
        pipeline_run(&pipeline, steps, n_ids ? ids : NULL, n_ids);

    } else if (strcmp(command, "sample") == 0) {
        IdList sample = {ids, 0};
        walk_dir(SAMPLE_DIR, collect_sample_id, &sample);
        qsort(ids, sample.count, sizeof(int), compare_ints);
        pipeline_init(&pipeline, &datalake, &index, &metadata, CONTROL_DIR, fetch_from_sample);
        pipeline_run(&pipeline, 2 * (int)sample.count, ids, sample.count);

    } else if (strcmp(command, "search") == 0) {
        char query[1024] = "";
        int phrase = 0;
        for (; arg < argc; arg++) {
            if (strcmp(argv[arg], "--phrase") == 0) {
                phrase = 1;
            } else {
                if (query[0]) strncat(query, " ", sizeof(query) - strlen(query) - 1);
                strncat(query, argv[arg], sizeof(query) - strlen(query) - 1);
            }
        }
        cmd_search(&index, &metadata, query, phrase);

    } else if (strcmp(command, "metadata") == 0) {
        const char *author = NULL, *language = NULL;
        int id = 0;
        for (; arg + 1 < argc; arg += 2) {
            if (strcmp(argv[arg], "--author") == 0) author = argv[arg + 1];
            else if (strcmp(argv[arg], "--language") == 0) language = argv[arg + 1];
            else if (strcmp(argv[arg], "--id") == 0) id = atoi(argv[arg + 1]);
        }
        cmd_metadata(&metadata, author, language, id);

    } else {
        usage();
    }

    disk_index_close(&index);
    metadata_close(&metadata);
    return 0;
}