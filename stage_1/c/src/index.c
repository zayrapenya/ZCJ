#include "index.h"

#include <stdlib.h>
#include <string.h>

#define GROW_ARRAY(array, count, capacity, initial)                           \
    do {                                                                      \
        if ((count) == (capacity)) {                                          \
            size_t _new_cap = (capacity) ? (capacity) * 2 : (initial);        \
            void *_bigger = realloc((array), _new_cap * sizeof(*(array)));    \
            if (!_bigger) return -1;                                          \
            (array) = _bigger;                                                \
            (capacity) = _new_cap;                                            \
        }                                                                     \
    } while (0)

InvertedIndex *index_create(void) {
    InvertedIndex *index = malloc(sizeof(InvertedIndex));
    if (!index) return NULL;
    index->terms = hashmap_create();
    if (!index->terms) { free(index); return NULL; }
    return index;
}

long index_add_book(InvertedIndex *index, int book_id, const TokenList *tokens) {
    long distinct = 0;

    for (size_t pos = 0; pos < tokens->count; pos++) {
        void **slot = hashmap_slot(index->terms, tokens->words[pos]);
        if (!slot) return -1;

        if (*slot == NULL) {
            *slot = calloc(1, sizeof(PostingList));
            if (!*slot) return -1;
        }
        PostingList *list = *slot;

        /* Tokens are read in order, so if the term already appeared in this
           book its posting is the last one of the list. */
        if (list->count == 0 || list->items[list->count - 1].book_id != book_id) {
            GROW_ARRAY(list->items, list->count, list->capacity, 2);
            Posting *posting = &list->items[list->count++];
            posting->book_id = book_id;
            posting->positions = NULL;
            posting->count = posting->capacity = 0;
            distinct++;
        }

        Posting *posting = &list->items[list->count - 1];
        GROW_ARRAY(posting->positions, posting->count, posting->capacity, 4);
        posting->positions[posting->count++] = (int)pos;
    }
    return distinct;
}

const PostingList *index_lookup(const InvertedIndex *index, const char *term) {
    return hashmap_get(index->terms, term);
}

const Posting *postings_find(const PostingList *list, int book_id) {
    if (!list) return NULL;
    for (size_t i = 0; i < list->count; i++) {
        if (list->items[i].book_id == book_id) return &list->items[i];
    }
    return NULL;
}

size_t index_term_count(const InvertedIndex *index) {
    return index->terms->count;
}

int index_put(InvertedIndex *index, const char *term, int book_id,
              const int *positions, size_t count) {
    void **slot = hashmap_slot(index->terms, term);
    if (!slot) return -1;
    if (*slot == NULL) {
        *slot = calloc(1, sizeof(PostingList));
        if (!*slot) return -1;
    }
    PostingList *list = *slot;

    Posting *posting = (Posting *)postings_find(list, book_id);
    if (!posting) {
        GROW_ARRAY(list->items, list->count, list->capacity, 2);
        posting = &list->items[list->count++];
        posting->book_id = book_id;
        posting->positions = NULL;
        posting->count = posting->capacity = 0;
    }

    int *copy = malloc((count ? count : 1) * sizeof(int));
    if (!copy) return -1;
    memcpy(copy, positions, count * sizeof(int));
    free(posting->positions);
    posting->positions = copy;
    posting->count = posting->capacity = count;
    return 0;
}

void free_postings(PostingList *list) {
    for (size_t i = 0; i < list->count; i++) free(list->items[i].positions);
    free(list->items);
    list->items = NULL;
    list->count = list->capacity = 0;
}

static void free_posting_list(void *value) {
    free_postings(value);
    free(value);
}

void index_free(InvertedIndex *index) {
    if (!index) return;
    hashmap_free(index->terms, free_posting_list);
    free(index);
}