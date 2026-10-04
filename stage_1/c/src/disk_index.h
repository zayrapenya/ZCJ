#ifndef DISK_INDEX_H
#define DISK_INDEX_H

#include <sqlite3.h>

#include "index.h"
#include "tokenizer.h"

#ifndef PATH_SIZE
#define PATH_SIZE 1024
#endif

/* The three on-disk structures, with the same files as the Python version:
     monolithic   -> datamarts/inverted_index.json  {"term":{"book":[positions]}}
     hierarchical -> datamarts/inverted_index/D/darcy.txt  one "book p1,p2,..." line per book
     sqlite       -> datamarts/inverted_index.db  table postings(term, book_id, positions) */
typedef enum { INDEX_MONOLITHIC, INDEX_HIERARCHICAL, INDEX_SQLITE } IndexKind;
#define N_INDEX_KINDS 3

typedef struct {
    IndexKind kind;
    char path[PATH_SIZE];
    InvertedIndex *data;    /* monolithic: whole index, loaded on first use */
    int dirty;
    sqlite3 *db;
    sqlite3_stmt *insert;
    sqlite3_stmt *select;
} DiskIndex;

typedef struct {
    long files;
    long long bytes;
} DiskUsage;

const char *index_kind_name(IndexKind kind);
int index_kind_parse(const char *name, IndexKind *out);

int disk_index_open(DiskIndex *index, IndexKind kind, const char *datamarts);

/* Adds (or updates) a book without rebuilding the index. */
int disk_index_add_book(DiskIndex *index, int book_id, const TokenList *tokens);

/* Writes pending changes. Only the monolithic file needs it. */
int disk_index_flush(DiskIndex *index);

/* Fills out with the postings of the term (empty if missing).
   The caller frees it with free_postings(). */
int disk_index_lookup(DiskIndex *index, const char *term, PostingList *out);

DiskUsage disk_index_usage(const DiskIndex *index);
void disk_index_close(DiskIndex *index);

#endif