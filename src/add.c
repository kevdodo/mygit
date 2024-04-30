#include "add.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "index_io.h"
#include "object_io.h"
#include "util.h"
#include <sys/stat.h>

#include <string.h>




void add_files(const char **file_paths, size_t file_count)
{
    // (void)file_paths;
    // (void)file_count;

    index_file_t * index_file = read_index_file();

    for (size_t i=0; i < file_count; i++){

        const char * file_path = file_paths[i];
        printf("file path: %s", file_path);
        index_entry_t *entry = (index_entry_t *) malloc(sizeof(index_entry_t));
        entry->fname = file_path;
        entry->fname_length = sizeof(file_path);

        struct stat file_stat;
        if (stat(file_path, &file_stat) == 0) {
            entry->size = file_stat.st_size;
        } else {
            // Handle error
            printf("something wrong with stat");
        }
        printf("whatat");

        FILE *file = fopen(file_path, "rb");
        if (file == NULL){
            perror("Could not open file");
        }

        // Get the size of the file
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        char *buffer = malloc(file_size);
        if (buffer == NULL) {
            fprintf(stderr, "Failed to allocate memory\n");
            fclose(file);
        }

        size_t bytes_read = fread(buffer, 1, file_size, file);
        if (bytes_read < file_size) {
            fprintf(stderr, "Failed to read file %s\n", file_path);
            free(buffer);
            fclose(file);
        }


        char hash[HASH_STRING_LENGTH + 1];
        get_object_hash(BLOB, (void *) buffer, file_size, hash);

        write_object(BLOB, (void * )buffer, file_size, hash);

        memcpy(entry->sha1, hash, sizeof(hash));


        hash_table_add(index_file->entries, file_path, entry);


        free(buffer);

        fclose(file);
    }

    FILE *file = fopen(".git/index", "wb");
    if (file == NULL) {
        // Handle error
        printf("Failed to open file for writing");
        return;
    }

    size_t written = fwrite(index_file, sizeof(index_file_t), 1, file);
    if (written != 1) {
        // Handle error
        printf("Failed to write index file");
        return;
    }

    fclose(file);
    printf("yoooo what is up guys\n");
    
    // printf("Not implemented.\n");
    exit(1);
}
