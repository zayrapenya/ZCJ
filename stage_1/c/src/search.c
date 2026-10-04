#include "search.h"

#include <stdlib.h>
#include <string.h>

#include "tokenizer.h"

static const Posting *find_book(const PostingList *list, int book_id) {
    return postings_find(list, book_id);
}

/* Positions are stored in increasing order, so binary search can be used. */
static int contains_position(const Posting *posting, int position) {
    size_t low = 0, high = posting->count;
    while (low < high) {
        size_t mid = (low + high) / 2;
        if (posting->positions[mid] == position) return 1;
        if (posting->positions[mid] < position) low = mid + 1;
        else high = mid;
    }
    return 0;
}

static int append_posting(PostingList *out, int book_id, const int *positions, size_t count) {
    if (out->count == out->capacity) {
        size_t new_cap = out->capacity ? out->capacity * 2 : 4;
        Posting *bigger = realloc(out->items, new_cap * sizeof(Posting));
        if (!bigger) return -1;
        out->items = bigger;
        out->capacity = new_cap;
    }
    int *copy = malloc((count ? count : 1) * sizeof(int));
    if (!copy) return -1;
    memcpy(copy, positions, count * sizeof(int));
    out->items[out->count++] = (Posting){book_id, copy, count, count};
    return 0;
}

static int compare_books(const void *a, const void *b) {
    int x = ((const Posting *)a)->book_id, y = ((const Posting *)b)->book_id;
    return (x > y) - (x < y);
}

static void sort_by_book(PostingList *list) {
    if (list->count > 1) qsort(list->items, list->count, sizeof(Posting), compare_books);
}

/* The query word goes through the same tokenizer as the books ("Darcy!" -> "darcy"). */
int search_word(DiskIndex *index, const char *word, PostingList *out) {
    out->items = NULL;
    out->count = out->capacity = 0;

    TokenList terms;
    if (tokenize(word, &terms) != 0) return -1;
    int rc = terms.count ? disk_index_lookup(index, terms.words[0], out) : 0;
    free_tokens(&terms);
    sort_by_book(out);
    return rc;
}

int search_and(DiskIndex *index, const char **words, size_t n_words, PostingList *out) {
    out->items = NULL;
    out->count = out->capacity = 0;
    if (n_words == 0) return 0;

    PostingList *results = calloc(n_words, sizeof(PostingList));
    if (!results) return -1;
    for (size_t i = 0; i < n_words; i++) search_word(index, words[i], &results[i]);

    for (size_t b = 0; b < results[0].count; b++) {
        const Posting *first = &results[0].items[b];
        int in_all = 1;
        for (size_t i = 1; i < n_words && in_all; i++) {
            in_all = find_book(&results[i], first->book_id) != NULL;
        }
        if (in_all) append_posting(out, first->book_id, first->positions, first->count);
    }

    for (size_t i = 0; i < n_words; i++) free_postings(&results[i]);
    free(results);
    return 0;
}

int search_phrase(DiskIndex *index, const char *phrase, PostingList *out) {
    out->items = NULL;
    out->count = out->capacity = 0;

    TokenList terms;
    if (tokenize(phrase, &terms) != 0) return -1;
    if (terms.count == 0) {
        free_tokens(&terms);
        return 0;
    }

    PostingList *postings = calloc(terms.count, sizeof(PostingList));
    const Posting **in_book = calloc(terms.count, sizeof(Posting *));
    size_t starts_capacity = 64;
    int *starts = malloc(starts_capacity * sizeof(int));
    if (!postings || !in_book || !starts) {
        free(postings); free(in_book); free(starts); free_tokens(&terms);
        return -1;
    }
    for (size_t i = 0; i < terms.count; i++) {
        disk_index_lookup(index, terms.words[i], &postings[i]);
    }

    for (size_t b = 0; b < postings[0].count; b++) {
        int book_id = postings[0].items[b].book_id;
        in_book[0] = &postings[0].items[b];

        /* The book must contain every word of the phrase */
        int in_all = 1;
        for (size_t i = 1; i < terms.count && in_all; i++) {
            in_book[i] = find_book(&postings[i], book_id);
            in_all = in_book[i] != NULL;
        }
        if (!in_all) continue;

        /* A match starts at s when word i is at position s + i for every i */
        size_t n_starts = 0;
        for (size_t p = 0; p < in_book[0]->count; p++) {
            int start = in_book[0]->positions[p];
            int match = 1;
            for (size_t i = 1; i < terms.count && match; i++) {
                match = contains_position(in_book[i], start + (int)i);
            }
            if (!match) continue;

            if (n_starts == starts_capacity) {
                starts_capacity *= 2;
                int *bigger = realloc(starts, starts_capacity * sizeof(int));
                if (!bigger) break;
                starts = bigger;
            }
            starts[n_starts++] = start;
        }
        if (n_starts > 0) append_posting(out, book_id, starts, n_starts);
    }

    for (size_t i = 0; i < terms.count; i++) free_postings(&postings[i]);
    free(postings);
    free(in_book);
    free(starts);
    free_tokens(&terms);
    sort_by_book(out);
    return 0;
}