#ifndef HASHMAP_H
#define HASHMAP_H

#include <stddef.h>

typedef struct {
    char *key;
    void *value;
} HashEntry;

typedef struct {
    HashEntry *entries;
    size_t capacity;
    size_t count;
} HashMap;

HashMap *hashmap_create(void);
void *hashmap_get(const HashMap *map, const char *key);

/* Returns the value slot for key, inserting it with NULL if it was missing. */
void **hashmap_slot(HashMap *map, const char *key);

int hashmap_next(const HashMap *map, size_t *it, const char **key, void **value);
void hashmap_free(HashMap *map, void (*free_value)(void *));

#endif