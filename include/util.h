#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "constants.h"
#include "hash_table.h"

#include "config_io.h"

struct list_node {
   list_node_t *next;
   void *value;
};
struct linked_list {
    list_node_t *head;
    list_node_t **tail;
};


uint8_t from_octal(char c);
uint8_t from_decimal(char c);
uint8_t from_hex(char c);

char to_hex(uint8_t value);

void hash_to_hex(const uint8_t hash_bytes[HASH_BYTES], object_hash_t hash_string);
void hex_to_hash(const object_hash_t hash_string, uint8_t hash_bytes[HASH_BYTES]);

// Reads a big-endian unsigned integer with the given number of bytes
size_t read_be(const uint8_t *bytes, size_t length);

// Writes a big-endian unsigned integer with the given number of bytes
void write_be(size_t value, uint8_t *bytes, size_t length);

bool starts_with(const char *string, const char *prefix);

void make_dirs(char *path);
void make_parent_dirs(char *path);

size_t get_file_size(FILE *f);
char *get_file_contents(const char *file_path);

void expand_tree(object_hash_t tree_hash, hash_table_t* hash_table, char *curr_chars);
bool is_valid_commit_hash(const char *hash);

char *get_url(config_section_t *remote);
char *get_last_dir(char *file_dir_name);

#endif // #ifndef UTIL_H
