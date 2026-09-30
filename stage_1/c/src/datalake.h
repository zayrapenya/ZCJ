#ifndef DATALAKE_H
#define DATALAKE_H

#include <stddef.h>
#include <time.h>

#include "book.h"

#define PATH_SIZE 1024

/* Same three layouts as the Python version:
     time  -> <root>/YYYYMMDD/HH/<id>.body.txt
     book  -> <root>/books/<id>/body.txt
     range -> <root>/ranges/000000-000999/<id>.body.txt */
typedef enum { LAYOUT_TIME, LAYOUT_BOOK, LAYOUT_RANGE } DatalakeLayout;

typedef struct {
    DatalakeLayout layout;
    char root[PATH_SIZE];
    int bucket_size;
} Datalake;

typedef struct {
    int *ids;
    size_t count;
} IdList;

typedef struct {
    long files;
    long dirs;
    long long bytes;
} StorageStats;

void datalake_init(Datalake *lake, DatalakeLayout layout, const char *root);
const char *datalake_name(DatalakeLayout layout);
int datalake_parse_layout(const char *name, DatalakeLayout *out);

int datalake_save(const Datalake *lake, int book_id, const Book *book, time_t now);

/* Fills the paths of the book files. Returns 0 if found, -1 otherwise. */
int datalake_locate(const Datalake *lake, int book_id, char *header_path, char *body_path);

int datalake_read(const Datalake *lake, int book_id, Book *out);

/* Scans the datalake and returns the ids of every stored book, sorted. */
int datalake_book_ids(const Datalake *lake, IdList *out);
void free_ids(IdList *list);

StorageStats datalake_stats(const Datalake *lake);

#endif