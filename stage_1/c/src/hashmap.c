#include "hashmap.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Open addressing with linear probing. Capacity is a power of two and the
   table doubles when it reaches 70% load. */

#define INITIAL_CAPACITY 1024

/* FNV-1a */
static uint64_t hash_key(const char *key) {
    uint64_t hash = 14695981039346656037ULL;
    for (const unsigned char *p = (const unsigned char *)key; *p; p++) {
        hash ^= *p;
        hash *= 1099511628211ULL;
    }
    return hash;
}

static HashEntry *find_entry(HashEntry *entries, size_t capacity, const char *key) {
    size_t mask = capacity - 1;
    size_t i = (size_t)hash_key(key) & mask;
    while (entries[i].key != NULL && strcmp(entries[i].key, key) != 0) {
        i = (i + 1) & mask;
    }
    return &entries[i];
}

HashMap *hashmap_create(void) {
    HashMap *map = malloc(sizeof(HashMap));
    if (!map) return NULL;
    map->capacity = INITIAL_CAPACITY;
    map->count = 0;
    map->entries = calloc(map->capacity, sizeof(HashEntry));
    if (!map->entries) { free(map); return NULL; }
    return map;
}

void *hashmap_get(const HashMap *map, const char *key) {
    HashEntry *entry = find_entry(map->entries, map->capacity, key);
    return entry->key ? entry->value : NULL;
}

static int grow(HashMap *map) {
    size_t new_capacity = map->capacity * 2;
    HashEntry *new_entries = calloc(new_capacity, sizeof(HashEntry));
    if (!new_entries) return -1;

    for (size_t i = 0; i < map->capacity; i++) {
        if (map->entries[i].key) {
            *find_entry(new_entries, new_capacity, map->entries[i].key) = map->entries[i];
        }
    }
    free(map->entries);
    map->entries = new_entries;
    map->capacity = new_capacity;
    return 0;
}

void **hashmap_slot(HashMap *map, const char *key) {
    HashEntry *entry = find_entry(map->entries, map->capacity, key);
    if (entry->key) return &entry->value;

    if ((map->count + 1) * 10 > map->capacity * 7) {
        if (grow(map) != 0) return NULL;
        entry = find_entry(map->entries, map->capacity, key);
    }

    size_t len = strlen(key);
    entry->key = malloc(len + 1);
    if (!entry->key) return NULL;
    memcpy(entry->key, key, len + 1);
    entry->value = NULL;
    map->count++;
    return &entry->value;
}

int hashmap_next(const HashMap *map, size_t *it, const char **key, void **value) {
    while (*it < map->capacity) {
        HashEntry *entry = &map->entries[(*it)++];
        if (entry->key) {
            *key = entry->key;
            *value = entry->value;
            return 1;
        }
    }
    return 0;
}

void hashmap_free(HashMap *map, void (*free_value)(void *)) {
    if (!map) return;
    for (size_t i = 0; i < map->capacity; i++) {
        if (map->entries[i].key) {
            free(map->entries[i].key);
            if (free_value) free_value(map->entries[i].value);
        }
    }
    free(map->entries);
    free(map);
}