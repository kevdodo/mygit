#include "util.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "index_io.h"
#include "object_io.h"
#include "ref_io.h"

#include "linked_list.h"
#include "hash_table.h"
#include "util.h"

#include "config_io.h"
#include <ctype.h>

uint8_t from_octal(char c) {
    if (!('0' <= c && c <= '7')) {
        fprintf(stderr, "Invalid octal character: %c\n", c);
        assert(false);
    }

    return c - '0';
}
uint8_t from_decimal(char c) {
    if (!('0' <= c && c <= '9')) {
        fprintf(stderr, "Invalid decimal character: %c\n", c);
        assert(false);
    }

    return c - '0';
}
uint8_t from_hex(char c) {
    if ('0' <= c && c <= '9') return c - '0';
    if ('a' <= c && c <= 'f') return 10 + c - 'a';

    fprintf(stderr, "Invalid hex character: '%c'\n", c);
    assert(false);
}

char to_hex(uint8_t value) {
    if (value < 10) return '0' + value;
    if (value < 16) return 'a' + value - 10;

    fprintf(stderr, "Invalid hex value: '%u'\n", value);
    assert(false);
}

void hash_to_hex(const uint8_t hash_bytes[HASH_BYTES], object_hash_t hash_string) {
    for (size_t i = 0; i < HASH_BYTES; i++) {
        hash_string[i * 2] = to_hex(hash_bytes[i] >> 4);
        hash_string[i * 2 + 1] = to_hex(hash_bytes[i] & 0xF);
    }
    hash_string[HASH_STRING_LENGTH] = '\0';
}

void hex_to_hash(const object_hash_t hash_string, uint8_t hash_bytes[HASH_BYTES]) {
    for (size_t i = 0; i < HASH_BYTES; i++) {
        hash_bytes[i] = from_hex(hash_string[i * 2]) << 4 |
                        from_hex(hash_string[i * 2 + 1]);
    }
}

size_t read_be(const uint8_t *bytes, size_t length) {
    size_t value = 0;
    while (length-- > 0) value = value << 8 | *(bytes++);
    return value;
}

void write_be(size_t value, uint8_t *bytes, size_t length) {
    while (length-- > 0) {
        bytes[length] = value;
        value >>= 8;
    }
    // Ensure that the entire value fit in the bytes
    assert(value == 0);
}

bool starts_with(const char *string, const char *prefix) {
    return !strncmp(string, prefix, strlen(prefix));
}

void make_dirs(char *path) {
    char *next = path;
    while ((next = strchr(next, '/')) != NULL) {
        *next = '\0';
        mkdir(path, 0755);
        *next = '/';
        next++;
    }
}
void make_parent_dirs(char *path) {
    char *file_start = strrchr(path, '/');
    if (file_start == NULL) return;

    // we want to include the trailing slash so make_dirs works properly
    // TODO: Not sure why these methods are separate to be honest
    file_start++; // safe, moves to null terminator in worst case
    char old_file_start = *file_start;
    *file_start = '\0';
    make_dirs(path);
    *file_start = old_file_start;
}

size_t get_file_size(FILE *f) {
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    rewind(f);
    return size;
}


char *get_file_contents(const char *file_path) {
    FILE *file = fopen(file_path, "rb");
    if (file == NULL) {
        // printf("Failed to open file");
        return NULL;
    }

    // Get the size of the file
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);


    // Allocate a buffer to hold the file contents
    char *buffer = malloc(file_size + 1);
    if (buffer == NULL) {
        perror("Failed to allocate memory");
        fclose(file);
        return NULL;
    }

    // Read the file into the buffer
    size_t bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read < (size_t) file_size) {
        // perror("Failed to read file");
        free(buffer);
        fclose(file);
        return NULL;
    }

    // Null-terminate the buffer
    buffer[file_size] = '\0';

    fclose(file);
    return buffer;
}


void expand_tree(object_hash_t tree_hash, hash_table_t* hash_table, char *curr_chars){
    // TODO: make not recursive cuz maybe they have a stack overflow
    tree_t *tree = read_tree(tree_hash);  // Call the read_tree function
    // printf("uhhhh\n");
    //lalallalalla

    for (size_t i = 0; i < tree->entry_count; i++) {
        tree_entry_t *entry = &tree->entries[i];
        if (entry->mode == MODE_DIRECTORY){
            // it's another tree object
            char *path = malloc(sizeof(char) * (strlen(entry->name) + strlen(curr_chars) + 2));
            strcpy(path, curr_chars);
            strcat(path, entry->name);
            strcat(path, "/");
            // printf("path: %s\n", path);
            // printf("TREE HASH: %s\n", entry->hash);
            expand_tree(entry->hash, hash_table, path);
            free(path);

        } else {
            char *path = malloc(sizeof(char) * (strlen(entry->name) + strlen(curr_chars) + 1));
            strcpy(path, curr_chars);
            strcat(path, entry->name);
            char *hash_copy = strdup(entry->hash);
            if (hash_copy == NULL) {
                // Handle error
            } else {
                hash_table_add(hash_table, path, hash_copy);
            }
            free(path);
        }
    }
    free_tree(tree);
}


bool is_valid_commit_hash(const char *hash) {
    // Check if the hash is 40 characters long
    if (strlen(hash) != 40) {
        return false;
    }

    // Check if all characters are hexadecimal
    for (int i = 0; i < 40; i++) {
        if (!isxdigit(hash[i])) {
            return false;
        }
    }

    return true;
}
    

char *get_url(config_section_t *remote) {
    for (size_t i = 0; i < remote->property_count; i++) {
        config_property_t property = remote->properties[i];
        if (strcmp(property.key, "url") == 0) {
            char *url = malloc(strlen(property.value) + 1);
            if (url == NULL) {
                fprintf(stderr, "Failed to allocate memory for URL.\n");
                return NULL;
            }
            strcpy(url, property.value);
            return url;
        }
    }
    return NULL;
}

char **split_path_into_directories(char *path) {
    // Count the number of directories in the path
    int count = 0;
    for (int i = 0; path[i]; i++) {
        if (path[i] == '/') {
            count++;
        }
    }

    // Allocate memory for the array of directory names
    char **directories = malloc((count + 2) * sizeof(char *));
    if (directories == NULL) {
        fprintf(stderr, "Failed to allocate memory.\n");
        return NULL;
    }

    // Split the path into directory names
    int index = 0;
    char *token = strtok(path, "/");
    while (token != NULL) {
        directories[index] = token;
        token = strtok(NULL, "/");
        index++;
    }
    directories[index] = NULL;  // Null-terminate the array

    return directories;
}

char *get_last_dir(char *file_dir_name){

    char * full_name = strdup(file_dir_name);     
    char ** full_name_splitted= split_path_into_directories(full_name);
    int count = 0;

    while (full_name_splitted[count] != NULL) {
        count++;
    }
    char *last_dir;
    if (count != 0){
        last_dir = strdup(full_name_splitted[count-1]);
    } else {
        last_dir = strdup(full_name);
    }
    free(full_name_splitted);
    free(full_name);
    return last_dir;
}

