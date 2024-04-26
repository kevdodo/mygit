#include "add.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "index_io.h"
#include "object_io.h"
#include "util.h"

typedef struct index_header_t{
    /* data */
    char signature[4];
    char version_num[4];
    uint32_t num_entries[4];
};


typedef struct index_file_t
{
    /* data */
    char signature[4];
    char version_num[4];
    uint32_t num_entries[4];
};


void add_files(const char **file_paths, size_t file_count)
{
    (void)file_paths;
    (void)file_count;
    printf("Not implemented.\n");
    exit(1);
}
