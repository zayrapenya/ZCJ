#include "disk_index.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "util.h"

static const char *KIND_NAMES[] = {"monolithic", "hierarchical", "sqlite"};

const char *index_kind_name(IndexKind kind) {
    return KIND_NAMES[kind];
}

int index_kind_parse(const char *name, IndexKind *out) {
    for (int i = 0; i < N_INDEX_KINDS; i++) {
        if (strcmp(name, KIND_NAMES[i]) == 0) {
            *out = (IndexKind)i;
            return 0;
        }
    }
    return -1;
}

/* ---------- Small helpers ---------- */

typedef struct {
    char *data;
    size_t len;
    size_t capacity;
} StrBuf;

static int buf_append(StrBuf *buf, const char *text, size_t len) {
    if (buf->len + len + 1 > buf->capacity) {
        size_t new_cap = buf->capacity ? buf->capacity * 2 : 4096;
        while (new_cap < buf->len + len + 1) new_cap *= 2;
        char *bigger = realloc(buf->data, new_cap);
        if (!bigger) return -1;
        buf->data = bigger;
        buf->capacity = new_cap;
    }
    memcpy(buf->data + buf->len, text, len);
    buf->len += len;
    buf->data[buf->len] = '\0';
    return 0;
}

static int buf_append_int(StrBuf *buf, int value) {
    char number[16];
    int len = snprintf(number, sizeof(number), "%d", value);
    return buf_append(buf, number, (size_t)len);
}

/* "10,250,7" */
static int buf_append_positions(StrBuf *buf, const Posting *posting) {
    for (size_t i = 0; i < posting->count; i++) {
        if (i > 0 && buf_append(buf, ",", 1) != 0) return -1;
        if (buf_append_int(buf, posting->positions[i]) != 0) return -1;
    }
    return 0;
}

/* Parses "10,250,7" and stores it as the positions of book_id in out. */
static const char *parse_positions(const char *p, int book_id, PostingList *out) {
    size_t count = 0, capacity = 16;
    int *positions = malloc(capacity * sizeof(int));
    if (!positions) return NULL;

    while (*p >= '0' && *p <= '9') {
        if (count == capacity) {
            capacity *= 2;
            int *bigger = realloc(positions, capacity * sizeof(int));
            if (!bigger) { free(positions); return NULL; }
            positions = bigger;
        }
        positions[count++] = (int)strtol(p, (char **)&p, 10);
        if (*p == ',') p++;
    }

    if (out->count == out->capacity) {
        size_t new_cap = out->capacity ? out->capacity * 2 : 4;
        Posting *bigger = realloc(out->items, new_cap * sizeof(Posting));
        if (!bigger) { free(positions); return NULL; }
        out->items = bigger;
        out->capacity = new_cap;
    }
    out->items[out->count++] = (Posting){book_id, positions, count, capacity};
    return p;
}

/* Positions of every term of a single book, grouped by term */
static InvertedIndex *book_postings(int book_id, const TokenList *tokens) {
    InvertedIndex *postings = index_create();
    if (postings && index_add_book(postings, book_id, tokens) < 0) {
        index_free(postings);
        return NULL;
    }
    return postings;
}

static void copy_postings(const PostingList *from, PostingList *to) {
    to->items = NULL;
    to->count = to->capacity = 0;
    if (!from || from->count == 0) return;

    to->items = malloc(from->count * sizeof(Posting));
    if (!to->items) return;
    for (size_t i = 0; i < from->count; i++) {
        const Posting *src = &from->items[i];
        int *positions = malloc((src->count ? src->count : 1) * sizeof(int));
        if (!positions) break;
        memcpy(positions, src->positions, src->count * sizeof(int));
        to->items[to->count++] = (Posting){src->book_id, positions, src->count, src->count};
    }
    to->capacity = to->count;
}

/* ---------- Monolithic JSON file ---------- */

static const char *skip_spaces(const char *p) {
    while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') p++;
    return p;
}

/* Reads a "string" without escapes (terms are a-z and keys are digits). */
static const char *parse_string(const char *p, char *out, size_t size) {
    p = skip_spaces(p);
    if (*p != '"') return NULL;
    p++;
    size_t len = 0;
    while (*p && *p != '"') {
        if (len + 1 < size) out[len++] = *p;
        p++;
    }
    out[len] = '\0';
    return *p == '"' ? p + 1 : NULL;
}

static const char *expect(const char *p, char c) {
    p = skip_spaces(p);
    return *p == c ? p + 1 : NULL;
}

static int load_json(InvertedIndex *data, const char *text) {
    char term[256], book[32];
    PostingList tmp = {NULL, 0, 0};

    const char *p = expect(text, '{');
    if (!p) return -1;
    p = skip_spaces(p);
    while (p && *p != '}') {
        if (!(p = parse_string(p, term, sizeof(term))) || !(p = expect(p, ':')) ||
            !(p = expect(p, '{'))) return -1;

        p = skip_spaces(p);
        while (p && *p != '}') {
            if (!(p = parse_string(p, book, sizeof(book))) || !(p = expect(p, ':')) ||
                !(p = expect(p, '['))) return -1;
            p = parse_positions(skip_spaces(p), atoi(book), &tmp);
            if (!p || !(p = expect(p, ']'))) return -1;

            Posting *posting = &tmp.items[tmp.count - 1];
            index_put(data, term, posting->book_id, posting->positions, posting->count);
            free_postings(&tmp);

            p = skip_spaces(p);
            if (*p == ',') p = skip_spaces(p + 1);
        }
        if (!p) return -1;
        p = skip_spaces(p + 1);
        if (*p == ',') p = skip_spaces(p + 1);
    }
    return p ? 0 : -1;
}

static int monolithic_load(DiskIndex *index) {
    if (index->data) return 0;
    index->data = index_create();
    if (!index->data) return -1;

    char *text = read_file(index->path, NULL);
    if (!text) return 0;  /* no file yet: empty index */
    int rc = load_json(index->data, text);
    free(text);
    return rc;
}

static int monolithic_add(DiskIndex *index, int book_id, const TokenList *tokens) {
    if (monolithic_load(index) != 0) return -1;
    InvertedIndex *postings = book_postings(book_id, tokens);
    if (!postings) return -1;

    size_t it = 0;
    const char *term;
    void *value;
    while (hashmap_next(postings->terms, &it, &term, &value)) {
        const Posting *posting = &((PostingList *)value)->items[0];
        index_put(index->data, term, book_id, posting->positions, posting->count);
    }
    index_free(postings);
    index->dirty = 1;
    return 0;
}

/* Every update rewrites the whole file, like json.dumps(..., separators=(",", ":")) */
static int monolithic_flush(DiskIndex *index) {
    if (!index->dirty) return 0;

    StrBuf buf = {NULL, 0, 0};
    buf_append(&buf, "{", 1);
    size_t it = 0;
    const char *term;
    void *value;
    int first_term = 1;
    while (hashmap_next(index->data->terms, &it, &term, &value)) {
        const PostingList *list = value;
        if (!first_term) buf_append(&buf, ",", 1);
        first_term = 0;
        buf_append(&buf, "\"", 1);
        buf_append(&buf, term, strlen(term));
        buf_append(&buf, "\":{", 3);
        for (size_t i = 0; i < list->count; i++) {
            if (i > 0) buf_append(&buf, ",", 1);
            buf_append(&buf, "\"", 1);
            buf_append_int(&buf, list->items[i].book_id);
            buf_append(&buf, "\":[", 3);
            buf_append_positions(&buf, &list->items[i]);
            buf_append(&buf, "]", 1);
        }
        buf_append(&buf, "}", 1);
    }
    buf_append(&buf, "}", 1);

    int rc = write_file(index->path, buf.data, buf.len);
    free(buf.data);
    if (rc == 0) index->dirty = 0;
    return rc;
}

static int monolithic_lookup(DiskIndex *index, const char *term, PostingList *out) {
    if (monolithic_load(index) != 0) return -1;
    copy_postings(index_lookup(index->data, term), out);
    return 0;
}

/* ---------- Hierarchical folders ---------- */

/* File names that Windows does not allow */
static int is_reserved(const char *term) {
    return strcmp(term, "con") == 0 || strcmp(term, "prn") == 0 ||
           strcmp(term, "aux") == 0 || strcmp(term, "nul") == 0;
}

static void term_path(const DiskIndex *index, const char *term, char *out) {
    char letter = (char)(term[0] - 'a' + 'A');
    snprintf(out, PATH_SIZE, "%s/%c/%s%s.txt", index->path, letter, term,
             is_reserved(term) ? "_" : "");
}

static int hierarchical_add(DiskIndex *index, int book_id, const TokenList *tokens) {
    InvertedIndex *postings = book_postings(book_id, tokens);
    if (!postings) return -1;

    int created[26] = {0};
    StrBuf line = {NULL, 0, 0};
    size_t it = 0;
    const char *term;
    void *value;
    int rc = 0;
    while (rc == 0 && hashmap_next(postings->terms, &it, &term, &value)) {
        int letter = term[0] - 'a';
        if (!created[letter]) {
            char dir[PATH_SIZE];
            snprintf(dir, sizeof(dir), "%s/%c", index->path, 'A' + letter);
            make_dirs(dir);
            created[letter] = 1;
        }

        /* Only the files of the terms of this book are touched */
        line.len = 0;
        buf_append_int(&line, book_id);
        buf_append(&line, " ", 1);
        buf_append_positions(&line, &((PostingList *)value)->items[0]);
        buf_append(&line, "\n", 1);

        char path[PATH_SIZE];
        term_path(index, term, path);
        rc = append_file(path, line.data);
    }
    free(line.data);
    index_free(postings);
    return rc;
}

static int hierarchical_lookup(DiskIndex *index, const char *term, PostingList *out) {
    out->items = NULL;
    out->count = out->capacity = 0;
    if (!term[0] || term[0] < 'a' || term[0] > 'z') return 0;

    char path[PATH_SIZE];
    term_path(index, term, path);
    char *text = read_file(path, NULL);
    if (!text) return 0;

    const char *p = text;
    while (p && *p) {
        int book_id = (int)strtol(p, (char **)&p, 10);
        if (*p == ' ') p++;
        p = parse_positions(p, book_id, out);
        while (p && (*p == '\n' || *p == '\r')) p++;
    }
    free(text);
    return p ? 0 : -1;
}

/* ---------- SQLite ---------- */

static int sqlite_open(DiskIndex *index) {
    if (sqlite3_open(index->path, &index->db) != SQLITE_OK) return -1;
    const char *schema =
        "CREATE TABLE IF NOT EXISTS postings ("
        "  term      TEXT    NOT NULL,"
        "  book_id   INTEGER NOT NULL,"
        "  positions TEXT    NOT NULL,"
        "  PRIMARY KEY (term, book_id)"
        ") WITHOUT ROWID";
    if (sqlite3_exec(index->db, schema, NULL, NULL, NULL) != SQLITE_OK) return -1;
    if (sqlite3_prepare_v2(index->db, "INSERT OR REPLACE INTO postings VALUES (?, ?, ?)",
                           -1, &index->insert, NULL) != SQLITE_OK) return -1;
    if (sqlite3_prepare_v2(index->db, "SELECT book_id, positions FROM postings WHERE term = ?",
                           -1, &index->select, NULL) != SQLITE_OK) return -1;
    return 0;
}

static int sqlite_add(DiskIndex *index, int book_id, const TokenList *tokens) {
    InvertedIndex *postings = book_postings(book_id, tokens);
    if (!postings) return -1;

    /* One transaction per book, like executemany() + commit() in Python */
    sqlite3_exec(index->db, "BEGIN", NULL, NULL, NULL);
    StrBuf positions = {NULL, 0, 0};
    size_t it = 0;
    const char *term;
    void *value;
    int rc = 0;
    while (rc == 0 && hashmap_next(postings->terms, &it, &term, &value)) {
        positions.len = 0;
        buf_append_positions(&positions, &((PostingList *)value)->items[0]);

        sqlite3_reset(index->insert);
        sqlite3_bind_text(index->insert, 1, term, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(index->insert, 2, book_id);
        sqlite3_bind_text(index->insert, 3, positions.data, (int)positions.len, SQLITE_TRANSIENT);
        if (sqlite3_step(index->insert) != SQLITE_DONE) rc = -1;
    }
    sqlite3_exec(index->db, rc == 0 ? "COMMIT" : "ROLLBACK", NULL, NULL, NULL);
    free(positions.data);
    index_free(postings);
    return rc;
}

static int sqlite_lookup(DiskIndex *index, const char *term, PostingList *out) {
    out->items = NULL;
    out->count = out->capacity = 0;

    sqlite3_reset(index->select);
    sqlite3_bind_text(index->select, 1, term, -1, SQLITE_TRANSIENT);
    int rc;
    while ((rc = sqlite3_step(index->select)) == SQLITE_ROW) {
        int book_id = sqlite3_column_int(index->select, 0);
        const char *positions = (const char *)sqlite3_column_text(index->select, 1);
        if (!parse_positions(positions, book_id, out)) return -1;
    }
    return rc == SQLITE_DONE ? 0 : -1;
}

/* ---------- Public interface ---------- */

int disk_index_open(DiskIndex *index, IndexKind kind, const char *datamarts) {
    memset(index, 0, sizeof(*index));
    index->kind = kind;
    make_dirs(datamarts);

    switch (kind) {
    case INDEX_MONOLITHIC:
        snprintf(index->path, sizeof(index->path), "%s/inverted_index.json", datamarts);
        return 0;
    case INDEX_HIERARCHICAL:
        snprintf(index->path, sizeof(index->path), "%s/inverted_index", datamarts);
        return make_dirs(index->path);
    case INDEX_SQLITE:
        snprintf(index->path, sizeof(index->path), "%s/inverted_index.db", datamarts);
        return sqlite_open(index);
    }
    return -1;
}

int disk_index_add_book(DiskIndex *index, int book_id, const TokenList *tokens) {
    switch (index->kind) {
    case INDEX_MONOLITHIC: return monolithic_add(index, book_id, tokens);
    case INDEX_HIERARCHICAL: return hierarchical_add(index, book_id, tokens);
    case INDEX_SQLITE: return sqlite_add(index, book_id, tokens);
    }
    return -1;
}

int disk_index_flush(DiskIndex *index) {
    return index->kind == INDEX_MONOLITHIC ? monolithic_flush(index) : 0;
}

int disk_index_lookup(DiskIndex *index, const char *term, PostingList *out) {
    switch (index->kind) {
    case INDEX_MONOLITHIC: return monolithic_lookup(index, term, out);
    case INDEX_HIERARCHICAL: return hierarchical_lookup(index, term, out);
    case INDEX_SQLITE: return sqlite_lookup(index, term, out);
    }
    return -1;
}

static int count_file(const char *path, const char *name, int is_dir,
                      long long size, void *ctx) {
    (void)path; (void)name;
    DiskUsage *usage = ctx;
    if (!is_dir) {
        usage->files++;
        usage->bytes += size;
    }
    return 0;
}

DiskUsage disk_index_usage(const DiskIndex *index) {
    DiskUsage usage = {0, 0};
    struct stat st;
    if (stat(index->path, &st) != 0) return usage;

    if (S_ISDIR(st.st_mode)) {
        walk_dir(index->path, count_file, &usage);
    } else {
        usage.files = 1;
        usage.bytes = (long long)st.st_size;
    }
    return usage;
}

void disk_index_close(DiskIndex *index) {
    disk_index_flush(index);
    if (index->data) index_free(index->data);
    if (index->db) {
        sqlite3_finalize(index->insert);
        sqlite3_finalize(index->select);
        sqlite3_close(index->db);
    }
    memset(index, 0, sizeof(*index));
}