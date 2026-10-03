#ifndef METADATA_H
#define METADATA_H

#include <sqlite3.h>
#include <stddef.h>

/* Fields parsed from the Gutenberg header. Missing fields are NULL. */
typedef struct {
    char *title;
    char *author;
    char *release_date;
    char *language;
} Metadata;

typedef struct {
    int book_id;
    Metadata meta;
    char *path;
} BookRow;

typedef struct {
    BookRow *rows;
    size_t count;
} BookRows;

/* SQLite table books(book_id, title, author, release_date, language, path),
   same schema as the Python version. */
typedef struct {
    sqlite3 *db;
    sqlite3_stmt *insert;
    sqlite3_stmt *by_id;
    sqlite3_stmt *by_author;
    sqlite3_stmt *by_language;
    sqlite3_stmt *path_by_title;
} MetadataStore;

int parse_header(const char *header, Metadata *out);
void free_metadata(Metadata *meta);

int metadata_open(MetadataStore *store, const char *db_path);
void metadata_close(MetadataStore *store);


int metadata_insert(MetadataStore *store, int book_id, const Metadata *meta,
                    const char *path, int commit);
int metadata_commit(MetadataStore *store);

/* Returns 0 if found, 1 if not found, -1 on error. */
int metadata_get(MetadataStore *store, int book_id, BookRow *out);
int metadata_by_author(MetadataStore *store, const char *author, BookRows *out);
int metadata_by_language(MetadataStore *store, const char *language, BookRows *out);

/* Returned string must be freed. NULL if the title does not exist. */
char *metadata_path_by_title(MetadataStore *store, const char *title);
long metadata_count(MetadataStore *store);

void free_book_row(BookRow *row);
void free_book_rows(BookRows *rows);

#endif