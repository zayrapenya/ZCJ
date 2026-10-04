#ifndef SEARCH_H
#define SEARCH_H

#include <stddef.h>

#include "disk_index.h"

/* Same queries as the Python SearchEngine. Results are sorted by book id and
   must be freed with free_postings(). */

/* Books containing the word and its positions. */
int search_word(DiskIndex *index, const char *word, PostingList *out);

/* Books containing every word (positions of the first word are returned). */
int search_and(DiskIndex *index, const char **words, size_t n_words, PostingList *out);

/* Books where the words appear consecutively; returns the start positions. */
int search_phrase(DiskIndex *index, const char *phrase, PostingList *out);

#endif