#ifndef DOWNLOADER_H
#define DOWNLOADER_H

typedef enum { FETCH_OK, FETCH_NOT_FOUND, FETCH_ERROR } FetchStatus;

/* Where the raw text of a book comes from. On FETCH_OK *text must be freed. */
typedef FetchStatus (*FetchFn)(int book_id, char **text);

/* Downloads https://www.gutenberg.org/cache/epub/<id>/pg<id>.txt */
FetchStatus download_book(int book_id, char **text);

/* Offline source: reads sample_data/pg<id>.txt */
FetchStatus fetch_from_sample(int book_id, char **text);

#endif