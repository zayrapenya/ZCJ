#include "datalake.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

static const char *LAYOUT_NAMES[] = {"time", "book", "range"};

void datalake_init(Datalake *lake, DatalakeLayout layout, const char *root) {
    lake->layout = layout;
    snprintf(lake->root, sizeof(lake->root), "%s", root);
    lake->bucket_size = 1000;
}

const char *datalake_name(DatalakeLayout layout) {
    return LAYOUT_NAMES[layout];
}

int datalake_parse_layout(const char *name, DatalakeLayout *out) {
    for (int i = 0; i < 3; i++) {
        if (strcmp(name, LAYOUT_NAMES[i]) == 0) {
            *out = (DatalakeLayout)i;
            return 0;
        }
    }
    return -1;
}

static void book_dir(const Datalake *lake, int book_id, time_t now, char *out) {
    switch (lake->layout) {
    case LAYOUT_TIME: {
        char stamp[16];
        strftime(stamp, sizeof(stamp), "%Y%m%d/%H", localtime(&now));
        snprintf(out, PATH_SIZE, "%s/%s", lake->root, stamp);
        break;
    }
    case LAYOUT_BOOK:
        snprintf(out, PATH_SIZE, "%s/books/%d", lake->root, book_id);
        break;
    case LAYOUT_RANGE: {
        int start = (book_id / lake->bucket_size) * lake->bucket_size;
        snprintf(out, PATH_SIZE, "%s/ranges/%06d-%06d", lake->root, start,
                 start + lake->bucket_size - 1);
        break;
    }
    }
}

static void file_paths(const Datalake *lake, const char *dir, int book_id,
                       char *header_path, char *body_path) {
    if (lake->layout == LAYOUT_BOOK) {
        snprintf(header_path, PATH_SIZE, "%s/header.txt", dir);
        snprintf(body_path, PATH_SIZE, "%s/body.txt", dir);
    } else {
        snprintf(header_path, PATH_SIZE, "%s/%d.header.txt", dir, book_id);
        snprintf(body_path, PATH_SIZE, "%s/%d.body.txt", dir, book_id);
    }
}

int datalake_save(const Datalake *lake, int book_id, const Book *book, time_t now) {
    char dir[PATH_SIZE], header_path[PATH_SIZE], body_path[PATH_SIZE];
    book_dir(lake, book_id, now, dir);
    if (make_dirs(dir) != 0) return -1;

    file_paths(lake, dir, book_id, header_path, body_path);
    if (write_file(header_path, book->header, strlen(book->header)) != 0) return -1;
    return write_file(body_path, book->body, strlen(book->body));
}

/* The download date of a book is unknown, so the time layout has to check
   every date/hour folder until it finds it. */
static int locate_time(const Datalake *lake, int book_id, char *header_path, char *body_path) {
    DIR *days = opendir(lake->root);
    if (!days) return -1;

    struct dirent *day;
    int found = -1;
    while (found != 0 && (day = readdir(days)) != NULL) {
        if (day->d_name[0] == '.') continue;
        char day_dir[PATH_SIZE];
        snprintf(day_dir, sizeof(day_dir), "%s/%s", lake->root, day->d_name);

        DIR *hours = opendir(day_dir);
        if (!hours) continue;
        struct dirent *hour;
        while ((hour = readdir(hours)) != NULL) {
            if (hour->d_name[0] == '.') continue;
            char hour_dir[PATH_SIZE];
            snprintf(hour_dir, sizeof(hour_dir), "%s/%s", day_dir, hour->d_name);
            file_paths(lake, hour_dir, book_id, header_path, body_path);
            if (path_exists(body_path)) { found = 0; break; }
        }
        closedir(hours);
    }
    closedir(days);
    return found;
}

int datalake_locate(const Datalake *lake, int book_id, char *header_path, char *body_path) {
    if (lake->layout == LAYOUT_TIME) return locate_time(lake, book_id, header_path, body_path);

    char dir[PATH_SIZE];
    book_dir(lake, book_id, 0, dir);
    file_paths(lake, dir, book_id, header_path, body_path);
    return path_exists(body_path) ? 0 : -1;
}

int datalake_read(const Datalake *lake, int book_id, Book *out) {
    char header_path[PATH_SIZE], body_path[PATH_SIZE];
    if (datalake_locate(lake, book_id, header_path, body_path) != 0) return -1;

    out->header = read_file(header_path, NULL);
    out->body = read_file(body_path, NULL);
    if (!out->header || !out->body) {
        free_book(out);
        return -1;
    }
    return 0;
}

typedef struct {
    int *ids;
    size_t count;
    size_t capacity;
} IdCollector;

static int add_id(IdCollector *c, int id) {
    if (c->count == c->capacity) {
        size_t new_cap = c->capacity ? c->capacity * 2 : 64;
        int *bigger = realloc(c->ids, new_cap * sizeof(int));
        if (!bigger) return -1;
        c->ids = bigger;
        c->capacity = new_cap;
    }
    c->ids[c->count++] = id;
    return 0;
}

static int collect_body_file(const char *path, const char *name, int is_dir,
                             long long size, void *ctx) {
    (void)path; (void)size;
    const char *suffix = ".body.txt";
    size_t len = strlen(name), suffix_len = strlen(suffix);
    if (is_dir || len <= suffix_len || strcmp(name + len - suffix_len, suffix) != 0) return 0;
    return add_id(ctx, atoi(name));
}

static int compare_ints(const void *a, const void *b) {
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}

int datalake_book_ids(const Datalake *lake, IdList *out) {
    IdCollector c = {NULL, 0, 0};

    if (lake->layout == LAYOUT_BOOK) {
        char books_dir[PATH_SIZE];
        snprintf(books_dir, sizeof(books_dir), "%s/books", lake->root);
        DIR *dir = opendir(books_dir);
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (entry->d_name[0] == '.') continue;
                char body[PATH_SIZE];
                snprintf(body, sizeof(body), "%s/%s/body.txt", books_dir, entry->d_name);
                if (path_exists(body) && add_id(&c, atoi(entry->d_name)) != 0) break;
            }
            closedir(dir);
        }
    } else {
        walk_dir(lake->root, collect_body_file, &c);
    }

    qsort(c.ids, c.count, sizeof(int), compare_ints);

    /* A book stored twice (e.g. after an interrupted run) counts once, like a set */
    size_t unique = 0;
    for (size_t i = 0; i < c.count; i++) {
        if (unique == 0 || c.ids[unique - 1] != c.ids[i]) c.ids[unique++] = c.ids[i];
    }
    out->ids = c.ids;
    out->count = unique;
    return 0;
}

void free_ids(IdList *list) {
    free(list->ids);
    list->ids = NULL;
    list->count = 0;
}

static int count_entry(const char *path, const char *name, int is_dir,
                       long long size, void *ctx) {
    (void)path; (void)name;
    StorageStats *stats = ctx;
    if (is_dir) {
        stats->dirs++;
    } else {
        stats->files++;
        stats->bytes += size;
    }
    return 0;
}

StorageStats datalake_stats(const Datalake *lake) {
    StorageStats stats = {0, 0, 0};
    walk_dir(lake->root, count_entry, &stats);
    return stats;
}