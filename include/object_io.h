#ifndef OBJECT_IO_H
#define OBJECT_IO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include "transport.h"

void *read_object(const object_hash_t hash, object_type_t *type, size_t *length);

typedef struct {
    object_hash_t tree_hash;
    size_t parents;
    object_hash_t *parent_hashes;
    char *author;
    time_t author_time;
    char *committer;
    time_t commit_time;
    char *message;
} commit_t;

commit_t *read_commit(const object_hash_t hash);
void free_commit(commit_t *);

typedef enum {
    MODE_DIRECTORY  = 0040000,
    MODE_FILE       = 0100644,
    MODE_EXECUTABLE = 0100755,
    MODE_SYMLINK    = 0120000
} file_mode_t;

typedef struct {
    file_mode_t mode;
    char *name; // file or directory name
    object_hash_t hash; // blob (for files) or tree (for directories)
} tree_entry_t;

typedef struct {
    size_t entry_count;
    tree_entry_t *entries;
} tree_t;

tree_t *read_tree(const object_hash_t hash);
void free_tree(tree_t *);

typedef struct {
    size_t length;
    uint8_t *contents;
} blob_t;

blob_t *read_blob(const object_hash_t hash);
void free_blob(blob_t *);

void apply_ref_delta(
    const object_hash_t base_hash,
    const uint8_t *data,
    size_t length,
    object_hash_t hash
);

void write_object(
    object_type_t type,
    const void *contents,
    size_t length,
    object_hash_t hash
);

void get_object_hash(
    object_type_t type,
    const void *contents,
    size_t length,
    object_hash_t hash
);

void *read_object(const object_hash_t hash, object_type_t *type, size_t *length);

void free_tree_entry(tree_entry_t *tree_entry);
void free_tree_mine(tree_t *tree);

#endif // #ifndef OBJECT_IO_H
