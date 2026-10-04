#include "benchmark.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "book.h"
#include "config.h"
#include "control.h"
#include "datalake.h"
#include "disk_index.h"
#include "memtrack.h"
#include "metadata.h"
#include "search.h"
#include "tokenizer.h"
#include "util.h"

#define FIRST_SYNTHETIC_ID 100000
#define MAX_SAMPLE 64
#define RESULTS_DIR "benchmarks/results"

/* ---------- Common helpers (common.py) ---------- */

typedef struct {
    int id;
    Book book;
} SampleBook;

static SampleBook sample[MAX_SAMPLE];
static size_t n_sample = 0;

static int collect_sample(const char *path, const char *name, int is_dir,
                          long long size, void *ctx) {
    (void)size; (void)ctx;
    int id;
    char ext[8];
    if (is_dir || n_sample >= MAX_SAMPLE || sscanf(name, "pg%d.%4s", &id, ext) != 2 ||
        strcmp(ext, "txt") != 0) return 0;

    char *text = read_file(path, NULL);
    if (text && split_book(text, &sample[n_sample].book) == 0) {
        sample[n_sample++].id = id;
    }
    free(text);
    return 0;
}

static int compare_sample(const void *a, const void *b) {
    int x = ((const SampleBook *)a)->id, y = ((const SampleBook *)b)->id;
    return (x > y) - (x < y);
}

/* Header and body of every sample book, sorted by id (11, 84, 98, 1342, 1661) */
static int load_sample_books(void) {
    if (n_sample > 0) return 0;
    walk_dir(SAMPLE_DIR, collect_sample, NULL);
    qsort(sample, n_sample, sizeof(SampleBook), compare_sample);
    if (n_sample == 0) {
        printf("No books found in %s\n", SAMPLE_DIR);
        return -1;
    }
    return 0;
}

/* Synthetic book i: id 100000 + i, content of sample book i % n_sample */
static const Book *synthetic_book(size_t i) {
    return &sample[i % n_sample].book;
}

/* Scratch data goes to the system temp folder (BENCH_WORKSPACE overrides it)
   so synced folders like OneDrive do not interfere with the measurements. */
static void fresh_workspace(const char *name, char *out) {
    const char *base = getenv("BENCH_WORKSPACE");
    char root[PATH_SIZE];
    if (base) {
        snprintf(root, sizeof(root), "%s", base);
    } else {
        const char *tmp = getenv("TEMP");
        if (!tmp) tmp = getenv("TMPDIR");
        if (!tmp) tmp = "/tmp";
        snprintf(root, sizeof(root), "%s/stage1_bench_c", tmp);
    }
    snprintf(out, PATH_SIZE, "%s/%s", root, name);
    remove_tree(out);
    make_dirs(out);
}

/* Deterministic random numbers (same sequence on every system) */
static unsigned long long rng_state;

static void rng_seed(unsigned long long seed) {
    rng_state = seed * 6364136223846793005ULL + 1442695040888963407ULL;
}

static size_t rng_below(size_t n) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return (size_t)(rng_state % n);
}

typedef struct {
    const char *benchmark;
    FILE *csv;
} Results;

static int results_open(Results *r, const char *benchmark) {
    r->benchmark = benchmark;
    make_dirs(RESULTS_DIR);
    char path[PATH_SIZE];
    snprintf(path, sizeof(path), "%s/c_%s.csv", RESULTS_DIR, benchmark);
    r->csv = fopen(path, "wb");
    if (!r->csv) return -1;
    fprintf(r->csv, "language,benchmark,structure,n_books,metric,value\n");
    return 0;
}

static void results_add(Results *r, const char *structure, int n_books,
                        const char *metric, double value) {
    fprintf(r->csv, "c,%s,%s,%d,%s,%.6f\n", r->benchmark, structure, n_books, metric, value);
    fflush(r->csv);
    printf("  %-13s n=%-6d %-28s %.6f\n", structure, n_books, metric, value);
}

static void results_close(Results *r) {
    fclose(r->csv);
    printf("Results saved to %s/c_%s.csv\n", RESULTS_DIR, r->benchmark);
}

/* ---------- Datalake benchmark (bench_datalake.py) ---------- */

#define BOOKS_PER_HOUR 10
#define LOOKUPS 500

static time_t start_time(void) {
    struct tm t = {0};
    t.tm_year = 2025 - 1900;
    t.tm_mon = 8;       /* September */
    t.tm_mday = 25;
    t.tm_isdst = -1;
    return mktime(&t);
}

static void ingest(Datalake *lake, const char *control, const int *ids, const size_t *books,
                   size_t count, time_t start) {
    for (size_t i = 0; i < count; i++) {
        datalake_save(lake, ids[i], synthetic_book(books[i]), start + 3600 * (time_t)(i / BOOKS_PER_HOUR));
        control_add(control, ids[i]);
    }
}

static size_t count_difference(const IdList *a, const IdList *b) {
    size_t count = 0;
    for (size_t i = 0; i < a->count; i++) count += !id_list_contains(b, a->ids[i]);
    return count;
}

static int count_body_file(const char *path, const char *name, int is_dir,
                           long long size, void *ctx) {
    (void)path; (void)size;
    if (!is_dir && strstr(name, "body")) (*(long *)ctx)++;
    return 0;
}

static void bench_datalake(const int *sizes, size_t n_sizes) {
    Results results;
    if (results_open(&results, "datalake") != 0) return;

    /* Raw texts are rebuilt so the write benchmark also measures the split */
    char *raw[MAX_SAMPLE];
    for (size_t s = 0; s < n_sample; s++) {
        const Book *b = &sample[s].book;
        size_t len = strlen(b->header) + strlen(b->body) + 128;
        raw[s] = malloc(len);
        snprintf(raw[s], len, "%s\n*** START OF THE PROJECT GUTENBERG EBOOK X ***\n%s\n"
                 "*** END OF THE PROJECT GUTENBERG EBOOK X ***", b->header, b->body);
    }
    time_t start = start_time();

    for (size_t si = 0; si < n_sizes; si++) {
        int n = sizes[si];
        int *ids = malloc(n * sizeof(int));
        size_t *books = malloc(n * sizeof(size_t));
        int lookup_ids[LOOKUPS];
        for (int i = 0; i < n; i++) {
            ids[i] = FIRST_SYNTHETIC_ID + i;
            books[i] = (size_t)i;
        }
        rng_seed(42);
        for (int i = 0; i < LOOKUPS; i++) lookup_ids[i] = ids[rng_below((size_t)n)];

        for (int layout = 0; layout < 3; layout++) {
            const char *name = datalake_name((DatalakeLayout)layout);
            char workspace[PATH_SIZE], root[PATH_SIZE], downloaded[PATH_SIZE], indexed[PATH_SIZE];
            char ws_name[64];
            snprintf(ws_name, sizeof(ws_name), "datalake_%s", name);
            fresh_workspace(ws_name, workspace);
            snprintf(root, sizeof(root), "%s/datalake", workspace);
            snprintf(downloaded, sizeof(downloaded), "%s/control", workspace);
            make_dirs(downloaded);
            snprintf(downloaded, sizeof(downloaded), "%s/control/downloaded_books.txt", workspace);
            snprintf(indexed, sizeof(indexed), "%s/control/indexed_books.txt", workspace);

            Datalake lake;
            datalake_init(&lake, (DatalakeLayout)layout, root);

            /* Write throughput (includes splitting the raw text, as in the real pipeline) */
            double t0 = now_seconds();
            for (int i = 0; i < n; i++) {
                Book book;
                split_book(raw[i % n_sample], &book);
                datalake_save(&lake, ids[i], &book, start + 3600 * (time_t)(i / BOOKS_PER_HOUR));
                control_add(downloaded, ids[i]);
                free_book(&book);
            }
            results_add(&results, name, n, "write_books_per_sec", n / (now_seconds() - t0));

            /* Lookup cost */
            char header_path[PATH_SIZE], body_path[PATH_SIZE];
            t0 = now_seconds();
            for (int i = 0; i < LOOKUPS; i++) {
                if (datalake_locate(&lake, lookup_ids[i], header_path, body_path) != 0) {
                    printf("Book %d not found!\n", lookup_ids[i]);
                }
            }
            results_add(&results, name, n, "lookup_avg_ms", 1000 * (now_seconds() - t0) / LOOKUPS);

            /* Incremental processing: half of the books are already indexed */
            for (int i = 0; i < n / 2; i++) control_add(indexed, ids[i]);

            IdList done, already, scanned;
            t0 = now_seconds();
            control_read(downloaded, &done);
            control_read(indexed, &already);
            size_t pending_control = count_difference(&done, &already);
            results_add(&results, name, n, "incremental_control_ms", 1000 * (now_seconds() - t0));
            free_ids(&done);
            free_ids(&already);

            t0 = now_seconds();
            datalake_book_ids(&lake, &scanned);
            control_read(indexed, &already);
            size_t pending_scan = count_difference(&scanned, &already);
            results_add(&results, name, n, "incremental_scan_ms", 1000 * (now_seconds() - t0));
            if (pending_control != pending_scan) printf("Control files and datalake scan disagree!\n");
            free_ids(&scanned);
            free_ids(&already);

            /* Storage overhead */
            StorageStats stats = datalake_stats(&lake);
            results_add(&results, name, n, "files", (double)stats.files);
            results_add(&results, name, n, "dirs", (double)stats.dirs);
            results_add(&results, name, n, "megabytes", stats.bytes / 1e6);
        }

        /* Recovery: interrupt after 60% of the books, restart one day later */
        for (int layout = 0; layout < 3; layout++) {
            const char *name = datalake_name((DatalakeLayout)layout);
            char workspace[PATH_SIZE], root[PATH_SIZE], control[PATH_SIZE], ws_name[64];
            snprintf(ws_name, sizeof(ws_name), "recovery_%s", name);
            fresh_workspace(ws_name, workspace);
            snprintf(root, sizeof(root), "%s/datalake", workspace);
            snprintf(control, sizeof(control), "%s/control", workspace);
            make_dirs(control);
            snprintf(control, sizeof(control), "%s/control/downloaded_books.txt", workspace);

            Datalake lake;
            datalake_init(&lake, (DatalakeLayout)layout, root);
            int cut = (int)(n * 0.6);
            ingest(&lake, control, ids, books, (size_t)cut, start);

            /* Crash: the next book was written to disk but not registered */
            datalake_save(&lake, ids[cut], synthetic_book(books[cut]), start);

            IdList done;
            control_read(control, &done);
            double t0 = now_seconds();
            int *remaining = malloc(n * sizeof(int));
            size_t *remaining_books = malloc(n * sizeof(size_t));
            size_t n_remaining = 0;
            for (int i = 0; i < n; i++) {
                if (!id_list_contains(&done, ids[i])) {
                    remaining[n_remaining] = ids[i];
                    remaining_books[n_remaining++] = books[i];
                }
            }
            ingest(&lake, control, remaining, remaining_books, n_remaining, start + 24 * 3600);
            results_add(&results, name, n, "recovery_ms", 1000 * (now_seconds() - t0));

            IdList stored;
            datalake_book_ids(&lake, &stored);
            long body_files = 0;
            walk_dir(root, count_body_file, &body_files);
            long lost = 0;
            for (int i = 0; i < n; i++) lost += !id_list_contains(&stored, ids[i]);
            results_add(&results, name, n, "recovery_duplicated_books", (double)(body_files - (long)stored.count));
            results_add(&results, name, n, "recovery_lost_books", (double)lost);

            free_ids(&stored);
            free_ids(&done);
            free(remaining);
            free(remaining_books);
        }
        free(ids);
        free(books);
    }

    for (size_t s = 0; s < n_sample; s++) free(raw[s]);
    results_close(&results);
}

/* ---------- Inverted index benchmark (bench_index.py) ---------- */

#define REPEAT 5

/* Same query workload in every language */
static const char *WORDS[] = {"darcy", "monster", "alice", "holmes", "love",
                              "the", "revolution", "creature", "queen", "zzzz"};
static const char *AND_QUERIES[][2] = {{"darcy", "love"}, {"monster", "night"},
                                       {"alice", "queen"}, {"holmes", "watson"}};
static const char *PHRASES[] = {"mr darcy", "sherlock holmes", "the white rabbit",
                                "it was the best of times"};
#define N_WORDS (sizeof(WORDS) / sizeof(WORDS[0]))
#define N_AND (sizeof(AND_QUERIES) / sizeof(AND_QUERIES[0]))
#define N_PHRASES (sizeof(PHRASES) / sizeof(PHRASES[0]))

static void run_queries(DiskIndex *index) {
    PostingList result;
    for (size_t i = 0; i < N_WORDS; i++) {
        search_word(index, WORDS[i], &result);
        free_postings(&result);
    }
    for (size_t i = 0; i < N_AND; i++) {
        search_and(index, AND_QUERIES[i], 2, &result);
        free_postings(&result);
    }
    for (size_t i = 0; i < N_PHRASES; i++) {
        search_phrase(index, PHRASES[i], &result);
        free_postings(&result);
    }
}

static void bench_index(const int *sizes, size_t n_sizes) {
    Results results;
    if (results_open(&results, "index") != 0) return;
    size_t n_queries = N_WORDS + N_AND + N_PHRASES;

    /* Every synthetic book is a copy of a sample book, so each sample book
       is tokenized once and reused */
    TokenList tokens[MAX_SAMPLE];
    for (size_t s = 0; s < n_sample; s++) tokenize(sample[s].book.body, &tokens[s]);

    for (size_t si = 0; si < n_sizes; si++) {
        int n = sizes[si];
        for (int kind = 0; kind < N_INDEX_KINDS; kind++) {
            const char *name = index_kind_name((IndexKind)kind);
            char workspace[PATH_SIZE], ws_name[64];
            snprintf(ws_name, sizeof(ws_name), "index_%s", name);
            fresh_workspace(ws_name, workspace);

            DiskIndex index;
            disk_index_open(&index, (IndexKind)kind, workspace);

            memtrack_reset_peak();
            double t0 = now_seconds();
            for (int i = 0; i < n; i++) {
                disk_index_add_book(&index, FIRST_SYNTHETIC_ID + i, &tokens[i % n_sample]);
            }
            disk_index_flush(&index);
            results_add(&results, name, n, "indexing_sec", now_seconds() - t0);
            results_add(&results, name, n, "indexing_peak_ram_mb", memtrack_peak() / 1e6);

            /* Reopen the index so queries read from storage, not from the build process */
            disk_index_close(&index);
            disk_index_open(&index, (IndexKind)kind, workspace);

            t0 = now_seconds();
            run_queries(&index);  /* cold: includes loading the index */
            results_add(&results, name, n, "query_cold_total_ms", 1000 * (now_seconds() - t0));

            t0 = now_seconds();
            for (int r = 0; r < REPEAT; r++) run_queries(&index);
            results_add(&results, name, n, "query_avg_ms",
                        1000 * (now_seconds() - t0) / (REPEAT * n_queries));

            t0 = now_seconds();
            disk_index_add_book(&index, FIRST_SYNTHETIC_ID + n, &tokens[n % n_sample]);
            disk_index_flush(&index);
            results_add(&results, name, n, "update_one_book_sec", now_seconds() - t0);

            DiskUsage usage = disk_index_usage(&index);
            results_add(&results, name, n, "disk_files", (double)usage.files);
            results_add(&results, name, n, "disk_mb", usage.bytes / 1e6);
            disk_index_close(&index);
        }
    }

    for (size_t s = 0; s < n_sample; s++) free_tokens(&tokens[s]);
    results_close(&results);
}

/* ---------- Metadata benchmark (bench_metadata.py) ---------- */

#define QUERIES 200
#define N_AUTHORS 500

static char *format_text(const char *base, const char *format, int value) {
    char buffer[512];
    snprintf(buffer, sizeof(buffer), format, base ? base : "None", value);
    size_t len = strlen(buffer);
    char *copy = malloc(len + 1);
    if (copy) memcpy(copy, buffer, len + 1);
    return copy;
}

static void bench_metadata(const int *sizes, size_t n_sizes) {
    Results results;
    if (results_open(&results, "metadata") != 0) return;

    Metadata templates[MAX_SAMPLE];
    for (size_t s = 0; s < n_sample; s++) parse_header(sample[s].book.header, &templates[s]);

    for (size_t si = 0; si < n_sizes; si++) {
        int n = sizes[si];
        Metadata *rows = malloc(n * sizeof(Metadata));
        char (*paths)[64] = malloc(n * sizeof(*paths));
        for (int i = 0; i < n; i++) {
            const Metadata *t = &templates[i % n_sample];
            rows[i].author = format_text(t->author, "%s %d", i % N_AUTHORS);
            rows[i].title = format_text(t->title, "%s #%d", i);
            rows[i].release_date = t->release_date;   /* shared, not freed */
            rows[i].language = t->language;
            snprintf(paths[i], sizeof(paths[i]), "datalake/books/%d/body.txt", i + 1);
        }

        char workspace[PATH_SIZE], db_path[PATH_SIZE];
        fresh_workspace("metadata", workspace);
        snprintf(db_path, sizeof(db_path), "%s/metadata.db", workspace);
        MetadataStore store;
        metadata_open(&store, db_path);

        double t0 = now_seconds();
        for (int i = 0; i < n; i++) metadata_insert(&store, i + 1, &rows[i], paths[i], 0);
        metadata_commit(&store);
        results_add(&results, "sqlite", n, "insert_rows_per_sec", n / (now_seconds() - t0));

        rng_seed(42);
        int picks[QUERIES];
        for (int q = 0; q < QUERIES; q++) picks[q] = (int)rng_below((size_t)n);

        t0 = now_seconds();
        for (int q = 0; q < QUERIES; q++) {
            BookRows found;
            metadata_by_author(&store, rows[picks[q]].author, &found);
            free_book_rows(&found);
        }
        results_add(&results, "sqlite", n, "query_by_author_ms", 1000 * (now_seconds() - t0) / QUERIES);

        t0 = now_seconds();
        for (int q = 0; q < QUERIES; q++) free(metadata_path_by_title(&store, rows[picks[q]].title));
        results_add(&results, "sqlite", n, "query_path_by_title_ms", 1000 * (now_seconds() - t0) / QUERIES);

        t0 = now_seconds();
        for (int q = 0; q < QUERIES; q++) {
            BookRow row;
            if (metadata_get(&store, picks[q] + 1, &row) == 0) free_book_row(&row);
        }
        results_add(&results, "sqlite", n, "query_by_id_ms", 1000 * (now_seconds() - t0) / QUERIES);

        metadata_close(&store);
        for (int i = 0; i < n; i++) {
            free(rows[i].author);
            free(rows[i].title);
        }
        free(rows);
        free(paths);
    }

    for (size_t s = 0; s < n_sample; s++) free_metadata(&templates[s]);
    results_close(&results);
}

/* ---------- Entry point ---------- */

int run_benchmark(const char *which, const int *sizes, size_t n_sizes) {
    static const int DATALAKE_SIZES[] = {50, 100, 200};
    static const int INDEX_SIZES[] = {5, 10, 20};
    static const int METADATA_SIZES[] = {100, 1000, 10000};

    if (load_sample_books() != 0) return 1;
    int all = strcmp(which, "all") == 0;
    int ran = 0;

    if (all || strcmp(which, "datalake") == 0) {
        printf("Datalake benchmark\n");
        bench_datalake(n_sizes ? sizes : DATALAKE_SIZES, n_sizes ? n_sizes : 3);
        ran = 1;
    }
    if (all || strcmp(which, "index") == 0) {
        printf("Index benchmark\n");
        bench_index(n_sizes ? sizes : INDEX_SIZES, n_sizes ? n_sizes : 3);
        ran = 1;
    }
    if (all || strcmp(which, "metadata") == 0) {
        printf("Metadata benchmark\n");
        bench_metadata(n_sizes ? sizes : METADATA_SIZES, n_sizes ? n_sizes : 3);
        ran = 1;
    }
    if (!ran) {
        printf("Usage: search_engine bench datalake|index|metadata|all [sizes...]\n");
        return 1;
    }
    return 0;
}