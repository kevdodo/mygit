#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdbool.h>
#include <stddef.h>
#include "linked_list.h"

typedef struct hash_table hash_table_t;
typedef struct key_value key_value_t;

void *value_of(const key_value_t *);
char *key_of(const key_value_t *);

hash_table_t *hash_table_init(void);
void free_hash_table(hash_table_t *, free_func_t);

void *hash_table_add(hash_table_t *, const char *key, void *value);
void *hash_table_get(const hash_table_t *, const char *key);
bool hash_table_contains(const hash_table_t *, const char *key);
size_t hash_table_size(const hash_table_t *);
void hash_table_sort(hash_table_t *);

list_node_t *key_set(const hash_table_t *);

#endif
