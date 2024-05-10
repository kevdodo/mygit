#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "hash_table.h"

// prime
const size_t BUCKETS = 7919;

// Entries in a hash table's linked list.
struct key_value {
    char *key;
    void *value;
};

struct hash_table {
    linked_list_t *entries[BUCKETS];

    // Number of keys. Handy to keep track of this as it needs to be written
    // into index file metadata.
    size_t size;

    // Maintain the key set so we can iterate over the hash table
    // TODO: point to key_value_t
    linked_list_t *key_set;
};

// Simple hash function
// h(s) = sum s_i * 31^i
size_t hash_func(const char *key) {
    size_t hash = 0;
    while (*key) hash = hash * 31 + *(key++);
    return hash;
}

// key + value pair functions

key_value_t *key_value_init(const char *key, void *value) {
    key_value_t *pair = malloc(sizeof(*pair));
    assert(pair != NULL);
    pair->key = strdup(key);
    assert(pair->key != NULL);
    pair->value = value;
    return pair;
}

void free_key_value(key_value_t *pair, free_func_t freer) {
    free(pair->key);
    if (freer) freer(pair->value);
    free(pair);
}

bool key_equals(const key_value_t *pair, const char *key) {
    return strcmp(pair->key, key) == 0;
}

char *key_of(const key_value_t *pair) {
    return pair->key;
}

void *value_of(const key_value_t *pair) {
    return pair->value;
}

// Hash table functions

hash_table_t *hash_table_init(void) {
    hash_table_t *table = calloc(1, sizeof(*table));
    assert(table != NULL);
    table->key_set = init_linked_list();
    return table;
}
void free_hash_table(hash_table_t *table, free_func_t freer) {
    for (size_t i = 0; i < BUCKETS; i++) {
        linked_list_t *list = table->entries[i];
        if (list != NULL) {
            // Annoying. Ideally, we would just call free_linked_list and pass
            // free_key_value. However, free_key_value is not the correct type;
            // it ALSO needs to be passed a freer for the (void *) value.
            for (list_node_t *it = list_head(list), *next; it != NULL; it = next) {
                free_key_value(node_value(it), freer);
                next = node_next(it);
                free(it);
            }
            free(list);
        }
    }
    free_linked_list(table->key_set, NULL);
    free(table);
}

void *hash_table_add(hash_table_t *table, const char *key, void *value) {
    linked_list_t **list = &table->entries[hash_func(key) % BUCKETS];
    if (*list == NULL) *list = init_linked_list();

    // If entry with passed key exists, modify its value.
    for (list_node_t *it = list_head(*list); it != NULL; it = node_next(it)) {
        key_value_t *pair = node_value(it);
        if (key_equals(pair, key)) {
            void *old_value = pair->value;
            pair->value = value;
            return old_value;
        }
    }

    // New entry if we make it here
    key_value_t *entry = key_value_init(key, value);
    list_push_back(*list, entry);
    list_push_back(table->key_set, entry->key);
    table->size++;
    return NULL;
}
void *hash_table_get(const hash_table_t *table, const char *key) {
    linked_list_t *list = table->entries[hash_func(key) % BUCKETS];
<<<<<<< HEAD

    if (list == NULL) return NULL;
    
=======
    if (list == NULL) return NULL;
>>>>>>> origin/start

    for (list_node_t *it = list_head(list); it != NULL; it = node_next(it)) {
        key_value_t *pair = node_value(it);
        if (key_equals(pair, key)) return value_of(pair);
    }
    return NULL;
}
bool hash_table_contains(const hash_table_t *table, const char *key) {
    return hash_table_get(table, key) != NULL;
}
size_t hash_table_size(const hash_table_t *table) {
    return table->size;
}
void hash_table_sort(hash_table_t *table) {
    list_sort(table->key_set, (compare_func_t) strcmp);
}

list_node_t *key_set(const hash_table_t *table) {
    return list_head(table->key_set);
}
