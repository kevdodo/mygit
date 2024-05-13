#include "add.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "index_io.h"
#include "object_io.h"
#include "util.h"
#include <sys/stat.h>


#include <string.h>
#include <unistd.h>



struct list_node {
   list_node_t *next;
   void *value;
};
struct linked_list {
    list_node_t *head;
    list_node_t **tail;
};
//

typedef struct index_header_t{
    char signature[4];
    uint32_t version_number;
    uint32_t number_of_entries;
}index_header_t;

typedef struct index_entry_full_t{
    uint32_t ctime_seconds;
    uint32_t ctime_nanoseconds;
    uint32_t mtime_seconds;
    uint32_t mtime_nanoseconds;
    uint32_t dev;
    uint32_t ino;
    // mode consists of unused (16 bits), object type (4 bits),
    // unused (3 bits), and UNIX perms (9 bits 0644 nonexecuteable, 0755 executeable)
    uint32_t mode;
    uint32_t uid;
    uint32_t gid;
    uint32_t file_size;
    char sha1_hash[HASH_BYTES];
    
    uint16_t flags;

    char *file_name;
    char null_byte;
}index_entry_full_t;

uint16_t get_flags(uint32_t fsize){
    if (fsize >= 0xfff){
        return 0xfff;
    }
    return (uint16_t) fsize;
}

index_entry_full_t *make_full_index_entry(index_entry_t *index_entry_temp){
    index_entry_full_t *index_entry = malloc(sizeof(index_entry_full_t));

    index_entry->ctime_seconds = 0;
    index_entry->ctime_nanoseconds = 0;

    struct stat file_stat;
    bool executable = false;
    // TODO: Should this be recalculated???
    if (stat(index_entry_temp->fname, &file_stat) == 0) {
        index_entry->mtime_seconds = (uint32_t)file_stat.st_mtime;

        if (file_stat.st_mode & S_IXUSR || file_stat.st_mode & S_IXGRP || file_stat.st_mode & S_IXOTH) {
            executable = true;
        } else {
            executable = false;
        }
    } else {
        // Handle error
        printf("something wrong with stat");
    }

    index_entry->mtime_nanoseconds = 0;
    index_entry->dev = 0;
    index_entry->ino = 0;
    
    index_entry->mode = 0b10000000000000000000000000000000;

    if (executable){
        index_entry->mode += 0b00000000000000000000000000000000110100100;
    } else {
        index_entry->mode += 0b00000000000000000000000000000000111101101;
    }
    
    index_entry->uid = 0;
    index_entry->gid = 0;

    index_entry->file_size = index_entry_temp->size;

    // copy the sha1 hash and file_name
    // index_entry->sha1_hash = malloc(sizeof(char) * HASH_BYTES);

    memcpy(index_entry->sha1_hash, index_entry_temp->sha1, HASH_BYTES);

    index_entry->flags = get_flags(index_entry_temp->fname_length);
    index_entry->file_name = malloc(sizeof(char)*(index_entry_temp->fname_length + 1));
    strncpy(index_entry->file_name, index_entry_temp->fname, index_entry_temp->fname_length);
    index_entry->file_name[index_entry_temp->fname_length] = '\0';

    index_entry->null_byte = '\0';

    return index_entry;
}

void free_full_index_entry(index_entry_full_t *index_entry){
    free(index_entry->file_name);
    free(index_entry);
}

size_t get_index_size(index_entry_full_t *index_entry){
    // Account for the fact that it's a char pointer, and we just store the 
    size_t index_size =  sizeof(index_entry_full_t) - sizeof(char *) - 
    sizeof(size_t) + strlen(index_entry->file_name);
    
    size_t padding;
    if (index_size % 8 == 0){
        padding = 0;
    } else {
        padding = 8 - (index_size % 8);
    }
    return index_size + padding;
}


void write_index(FILE *f, index_entry_full_t *index_entry){
    size_t index_size = get_index_size(index_entry);

    fwrite(&index_entry->ctime_seconds, sizeof(uint32_t), 1, f);
    fwrite(&index_entry->ctime_nanoseconds, sizeof(uint32_t), 1, f);

    uint8_t mtime_seconds[4];
    printf("mtime: %u", index_entry->mtime_seconds);
    write_be(index_entry->mtime_seconds, mtime_seconds, 4);
    fwrite(&mtime_seconds, sizeof(uint32_t), 1, f);

    fwrite(&index_entry->mtime_nanoseconds, sizeof(uint32_t), 1, f);

    fwrite(&index_entry->dev, sizeof(uint32_t), 1, f);
    fwrite(&index_entry->ino, sizeof(uint32_t), 1, f);

    uint8_t mode[4];
    write_be(index_entry->mode, mode, 4);
    fwrite(&mode, sizeof(uint32_t), 1, f);
    
    // fwrite(&index_entry->mode, sizeof(uint32_t), 1, f);
    fwrite(&index_entry->uid, sizeof(uint32_t), 1, f);
    fwrite(&index_entry->gid, sizeof(uint32_t), 1, f);

    uint8_t file_size[4];
    write_be(index_entry->file_size, file_size, 4);
    fwrite(&file_size, sizeof(uint32_t), 1, f);

    // fwrite(&index_entry->file_size, sizeof(uint32_t), 1, f);
    
    uint8_t sha1_hash[HASH_BYTES];
    write_be(index_entry->sha1_hash, sha1_hash, HASH_BYTES);
    fwrite(&sha1_hash, sizeof(char), HASH_BYTES, f);

    // fwrite(&index_entry->sha1_hash, sizeof(char), HASH_BYTES, f);

    uint8_t flags[2];
    write_be(index_entry->flags, flags, 2);
    // fwrite(&flags, sizeof(char), HASH_BYTES, f);
    fwrite(&flags, sizeof(uint16_t), 1, f);


    // fwrite(index_entry->file_name, sizeof(char), strlen(index_entry->file_name), f);

}

index_entry_t *make_index_entry(size_t size, object_hash_t sha1, char *fname, uint32_t fname_length, time_t mtime){
    index_entry_t *index_entry = malloc(sizeof(index_entry_t));
    index_entry->size = size;
    // need to copy the sha1 hash
    memcpy(index_entry->sha1, sha1, sizeof(object_hash_t));
    strncpy(index_entry->fname, fname, fname_length);
    index_entry->fname[fname_length] = '\0';
    index_entry->mtime = mtime;
    return index_entry;
}


char *get_contents(const char *file_path){
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
    if (bytes_read < (size_t) file_size) {
        fprintf(stderr, "Failed to read file %s\n", file_path);
        free(buffer);
        fclose(file);
    }

    fclose(file);

    return buffer;
}

void write_index_header(FILE *f, uint32_t num_entries){
    // for the null character
    fwrite("DIRC", sizeof(char), 4, f);
    uint32_t version_number = 2;
    uint8_t bytes[4];
    write_be(version_number, bytes, 4);
    fwrite(&bytes, sizeof(uint32_t), 1, f);

    uint8_t num_entry_bytes[4];
    write_be(num_entries, num_entry_bytes, 4);
    fwrite(&num_entry_bytes, sizeof(uint32_t), 1, f);
}


uint32_t get_idx_file_cnts(const char **file_paths, size_t file_count, hash_table_t *index_table){
    uint32_t index_file_cnts = hash_table_size(index_table);
    for (size_t i=0; i < file_count; i++){
        const char *file_path = file_paths[i];
        printf("filename: %s\n", file_path);
        // hash_table_add(index_table, file_path, "bruh");
        if (hash_table_contains(index_table, file_path)){
            printf("table contains %s", file_path);
            if (access(file_path, F_OK) == -1){
                // file was deleted
                printf("not here\n");
                index_file_cnts--;
            } else{
                // file is still there
                printf("its here\n");
                index_file_cnts++;
            }
        }
    }
    return index_file_cnts;
}



void add_files(const char **file_paths, size_t file_count)
{
    index_file_t * index_file = read_index_file();

    hash_table_t *index_table = index_file->entries;


    uint32_t index_cnts = get_idx_file_cnts(file_paths, file_count, index_table);
    printf("index_cnts: %u\n", index_cnts);
    FILE *new_index_file = fopen("temp_idx_file", "wb");

    write_index_header(new_index_file, index_cnts);


    uint32_t index_file_cnts = hash_table_size(index_table);
    for (size_t i=0; i < file_count; i++){
        const char *file_path = file_paths[i];
        printf("filename: %s\n", file_path);
        // hash_table_add(index_table, file_path, "bruh");
        if (hash_table_contains(index_table, file_path)){
            if (access(file_path, F_OK) == -1){
                // file was deleted
                printf("not here\n");
                index_file_cnts--;
            } else{
                // file is still there
                printf("its here\n");
                index_file_cnts++;
                index_entry_full_t *full_idx = (hash_table_get(index_table, file_path));

                char *file_contents = get_file_contents(file_path);

                write_index(new_index_file, full_idx);

                object_hash_t hash;
                write_object(BLOB, file_contents, strlen(file_contents), hash);
            }
        } else {
            // new file
            if (access(file_path, F_OK) == -1){
                // file was deleted
                printf("not here\n");
                index_file_cnts--;
            } else{

                char *file_contents = get_file_contents(file_path);

                object_hash_t hash;
                write_object(BLOB, file_contents, strlen(file_contents), hash);
            }
        }
    }

    printf("yoooo what is up guys\n");
    
    exit(1);
}
