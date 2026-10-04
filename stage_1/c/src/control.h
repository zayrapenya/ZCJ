#ifndef CONTROL_H
#define CONTROL_H

#include <stddef.h>

#include "datalake.h"
#include "disk_index.h"
#include "downloader.h"
#include "metadata.h"

/* Minimal control layer, same as the Python version:
     control/downloaded_books.txt  books stored in the datalake
     control/indexed_books.txt     books added to the index and the metadata datamart
     control/failed_books.txt      ids that do not exist or have no Gutenberg markers
   An id is appended only after its step has finished, so an interrupted run
   simply repeats that step: no book is lost or stored twice. */

/* Ids of a control file, sorted. Missing file = empty list. */
int control_read(const char *path, IdList *out);
int control_add(const char *path, int book_id);
int id_list_contains(const IdList *list, int book_id);

typedef struct {
    Datalake *datalake;
    DiskIndex *index;
    MetadataStore *metadata;
    FetchFn fetch;
    char downloaded[PATH_SIZE];
    char indexed[PATH_SIZE];
    char failed[PATH_SIZE];
} Pipeline;

void pipeline_init(Pipeline *p, Datalake *datalake, DiskIndex *index,
                   MetadataStore *metadata, const char *control_dir, FetchFn fetch);

/* Return 1 on success, 0 if the book was skipped or failed. */
int pipeline_download(Pipeline *p, int book_id);
int pipeline_index_book(Pipeline *p, int book_id);

/* Downloaded but not yet indexed */
int pipeline_pending(Pipeline *p, IdList *out);

/* Indexes one pending book or, if there is none, downloads a new one.
   candidates can be NULL to try random ids. */
void pipeline_step(Pipeline *p, const int *candidates, size_t n_candidates);
void pipeline_run(Pipeline *p, int steps, const int *candidates, size_t n_candidates);

#endif