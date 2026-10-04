#ifndef INDEX_H
#define INDEX_H

#include <stddef.h>

#include "hashmap.h"
#include "tokenizer.h"

typedef struct {
    int book_id;
    int *positions;
    size_t count;
    size_t capacity;
} Posting;

typedef struct {
    Posting *items;
    size_t count;
    size_t capacity;
} PostingList;

/* In-memory inverted index: term -> PostingList* */
typedef struct {
    HashMap *terms;
} InvertedIndex;

InvertedIndex *index_create(void);

/* Returns the number of distinct terms in the book, or -1 on error. */
long index_add_book(InvertedIndex *index, int book_id, const TokenList *tokens);

const PostingList *index_lookup(const InvertedIndex *index, const char *term);
const Posting *postings_find(const PostingList *list, int book_id);
size_t index_term_count(const InvertedIndex *index);

/* Sets the positions of a term in a book (replacing them if the book was
   already there). Positions are copied. Returns 0 on success. */
int index_put(InvertedIndex *index, const char *term, int book_id,
              const int *positions, size_t count);

void free_postings(PostingList *list);

void index_free(InvertedIndex *index);

#endif