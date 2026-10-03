#include "metadata.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

static int is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\v' || c == '\f';
}

static char *copy_range(const char *start, const char *end) {
    while (start < end && is_space(*start)) start++;
    while (end > start && is_space(end[-1])) end--;
    if (start == end) return NULL;

    size_t len = (size_t)(end - start);
    char *copy = malloc(len + 1);
    if (!copy) return NULL;
    memcpy(copy, start, len);
    copy[len] = '\0';
    return copy;
}

/* Equivalent to the Python regex ^<prefix>\s*(.+)$ : value of the first line
   that starts with the prefix. */
static char *find_field(const char *header, const char *prefix) {
    size_t prefix_len = strlen(prefix);
    const char *line = header;
    while (*line) {
        const char *end = strchr(line, '\n');
        if (!end) end = line + strlen(line);
        if (strncmp(line, prefix, prefix_len) == 0) {
            return copy_range(line + prefix_len, end);
        }
        line = *end ? end + 1 : end;
    }
    return NULL;
}

int parse_header(const char *header, Metadata *out) {
    out->title = find_field(header, "Title:");
    out->author = find_field(header, "Author:");
    out->language = find_field(header, "Language:");

    out->release_date = find_field(header, "Release Date:");
    if (!out->release_date) out->release_date = find_field(header, "Release date:");

    /* "October 1, 1993 [eBook #84]" -> "October 1, 1993" */
    char *date = out->release_date;
    if (date && date[strlen(date) - 1] == ']') {
        char *bracket = strchr(date + 1, '[');
        if (bracket) {
            char *trimmed = copy_range(date, bracket);
            free(date);
            out->release_date = trimmed;
        }
    }
    return 0;
}

void free_metadata(Metadata *meta) {
    free(meta->title);
    free(meta->author);
    free(meta->release_date);
    free(meta->language);
    meta->title = meta->author = meta->release_date = meta->language = NULL;
}

static int prepare(MetadataStore *store, const char *sql, sqlite3_stmt **stmt) {
    return sqlite3_prepare_v2(store->db, sql, -1, stmt, NULL) == SQLITE_OK ? 0 : -1;
}

int metadata_open(MetadataStore *store, const char *db_path) {
    memset(store, 0, sizeof(*store));

    /* Create the parent folder, like Path.mkdir(parents=True) */
    char dir[1024];
    snprintf(dir, sizeof(dir), "%s", db_path);
    char *slash = strrchr(dir, '/');
    if (slash) {
        *slash = '\0';
        make_dirs(dir);
    }

    if (sqlite3_open(db_path, &store->db) != SQLITE_OK) return -1;

    const char *schema =
        "CREATE TABLE IF NOT EXISTS books ("
        "  book_id      INTEGER PRIMARY KEY,"
        "  title        TEXT,"
        "  author       TEXT,"
        "  release_date TEXT,"
        "  language     TEXT,"
        "  path         TEXT);"
        "CREATE INDEX IF NOT EXISTS idx_author ON books(author);"
        "CREATE INDEX IF NOT EXISTS idx_title ON books(title);";
    if (sqlite3_exec(store->db, schema, NULL, NULL, NULL) != SQLITE_OK) return -1;

    if (prepare(store, "INSERT OR REPLACE INTO books VALUES (?, ?, ?, ?, ?, ?)", &store->insert) ||
        prepare(store, "SELECT * FROM books WHERE book_id = ?", &store->by_id) ||
        prepare(store, "SELECT * FROM books WHERE author LIKE ?", &store->by_author) ||
        prepare(store, "SELECT * FROM books WHERE language = ?", &store->by_language) ||
        prepare(store, "SELECT path FROM books WHERE title = ?", &store->path_by_title)) {
        return -1;
    }
    return 0;
}

void metadata_close(MetadataStore *store) {
    if (!store->db) return;
    metadata_commit(store);
    sqlite3_finalize(store->insert);
    sqlite3_finalize(store->by_id);
    sqlite3_finalize(store->by_author);
    sqlite3_finalize(store->by_language);
    sqlite3_finalize(store->path_by_title);
    sqlite3_close(store->db);
    store->db = NULL;
}

/* NULL values are stored as SQL NULL, like None in Python */
static void bind_text(sqlite3_stmt *stmt, int index, const char *value) {
    if (value) {
        sqlite3_bind_text(stmt, index, value, -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, index);
    }
}

int metadata_insert(MetadataStore *store, int book_id, const Metadata *meta,
                    const char *path, int commit) {
    /* Python's sqlite3 opens a transaction automatically before an INSERT */
    if (sqlite3_get_autocommit(store->db)) {
        sqlite3_exec(store->db, "BEGIN", NULL, NULL, NULL);
    }

    sqlite3_stmt *stmt = store->insert;
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, book_id);
    bind_text(stmt, 2, meta->title);
    bind_text(stmt, 3, meta->author);
    bind_text(stmt, 4, meta->release_date);
    bind_text(stmt, 5, meta->language);
    bind_text(stmt, 6, path);
    if (sqlite3_step(stmt) != SQLITE_DONE) return -1;

    return commit ? metadata_commit(store) : 0;
}

int metadata_commit(MetadataStore *store) {
    if (sqlite3_get_autocommit(store->db)) return 0;
    return sqlite3_exec(store->db, "COMMIT", NULL, NULL, NULL) == SQLITE_OK ? 0 : -1;
}

static char *column_text(sqlite3_stmt *stmt, int column) {
    const unsigned char *text = sqlite3_column_text(stmt, column);
    if (!text) return NULL;
    size_t len = strlen((const char *)text);
    char *copy = malloc(len + 1);
    if (copy) memcpy(copy, text, len + 1);
    return copy;
}

static void read_row(sqlite3_stmt *stmt, BookRow *row) {
    row->book_id = sqlite3_column_int(stmt, 0);
    row->meta.title = column_text(stmt, 1);
    row->meta.author = column_text(stmt, 2);
    row->meta.release_date = column_text(stmt, 3);
    row->meta.language = column_text(stmt, 4);
    row->path = column_text(stmt, 5);
}

int metadata_get(MetadataStore *store, int book_id, BookRow *out) {
    sqlite3_stmt *stmt = store->by_id;
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, book_id);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        read_row(stmt, out);
        return 0;
    }
    return rc == SQLITE_DONE ? 1 : -1;
}

static int read_rows(sqlite3_stmt *stmt, BookRows *out) {
    size_t capacity = 0;
    out->rows = NULL;
    out->count = 0;

    int rc;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (out->count == capacity) {
            capacity = capacity ? capacity * 2 : 8;
            BookRow *bigger = realloc(out->rows, capacity * sizeof(BookRow));
            if (!bigger) return -1;
            out->rows = bigger;
        }
        read_row(stmt, &out->rows[out->count++]);
    }
    return rc == SQLITE_DONE ? 0 : -1;
}

int metadata_by_author(MetadataStore *store, const char *author, BookRows *out) {
    char pattern[512];
    snprintf(pattern, sizeof(pattern), "%%%s%%", author);

    sqlite3_stmt *stmt = store->by_author;
    sqlite3_reset(stmt);
    sqlite3_bind_text(stmt, 1, pattern, -1, SQLITE_TRANSIENT);
    return read_rows(stmt, out);
}

int metadata_by_language(MetadataStore *store, const char *language, BookRows *out) {
    sqlite3_stmt *stmt = store->by_language;
    sqlite3_reset(stmt);
    sqlite3_bind_text(stmt, 1, language, -1, SQLITE_TRANSIENT);
    return read_rows(stmt, out);
}

char *metadata_path_by_title(MetadataStore *store, const char *title) {
    sqlite3_stmt *stmt = store->path_by_title;
    sqlite3_reset(stmt);
    sqlite3_bind_text(stmt, 1, title, -1, SQLITE_TRANSIENT);
    return sqlite3_step(stmt) == SQLITE_ROW ? column_text(stmt, 0) : NULL;
}

long metadata_count(MetadataStore *store) {
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(store->db, "SELECT COUNT(*) FROM books", -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    long count = sqlite3_step(stmt) == SQLITE_ROW ? sqlite3_column_int64(stmt, 0) : -1;
    sqlite3_finalize(stmt);
    return count;
}

void free_book_row(BookRow *row) {
    free_metadata(&row->meta);
    free(row->path);
    row->path = NULL;
}

void free_book_rows(BookRows *rows) {
    for (size_t i = 0; i < rows->count; i++) free_book_row(&rows->rows[i]);
    free(rows->rows);
    rows->rows = NULL;
    rows->count = 0;
}