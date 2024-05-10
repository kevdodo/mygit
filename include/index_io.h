#ifndef INDEX_IO_H
#define INDEX_IO_H

#include <stdint.h>
#include <time.h>
#include "constants.h"
#include "hash_table.h"
#include "object_io.h"

typedef struct index_entry {
    uint32_t size;
    object_hash_t sha1;
    char *fname;
    uint32_t fname_length;
    time_t mtime;
<<<<<<< HEAD
=======
    uint32_t mode;
>>>>>>> origin/start
} index_entry_t;

typedef struct index_file {
    // Maps filenames to their index_entry
    hash_table_t *entries;
} index_file_t;

index_file_t *empty_index_file(void);
index_file_t *read_index_file(void);
void free_index_file(index_file_t *);
void free_index_entry(index_entry_t *);

#endif
