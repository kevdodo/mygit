#include "status.h"
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "index_io.h"
#include "object_io.h"
#include "ref_io.h"

#include "linked_list.h"

#include "util.h"

struct list_node {
   list_node_t *next;
   void *value;
};
struct linked_list {
    list_node_t *head;
    list_node_t **tail;
};


void expand_tree(object_hash_t tree_hash, hash_table_t* hash_table, char *curr_chars){
    
    tree_t *tree = read_tree(tree_hash);  // Call the read_tree function

    for (int i = 0; i < tree->entry_count; i++) {
        tree_entry_t *entry = &tree->entries[i];
        if (entry->mode == MODE_DIRECTORY){
            // it's another tree object
            char *path = malloc(sizeof(char) * (strlen(entry->name) + strlen(curr_chars) + 2));
            strcpy(path, curr_chars);
            strcat(path, entry->name);
            strcat(path, "/");
            printf("path: %s\n", path);
            printf("TREE HASH: %s\n", entry->hash);
            expand_tree(entry->hash, hash_table, path);
            free(path);

        } else {

            char *path = malloc(sizeof(char) * (strlen(entry->name) + strlen(curr_chars) + 1));
            strcpy(path, curr_chars);
            strcat(path, entry->name);

            // printf("final path: %s\n", path);

            // printf("jawn is in the commit %s\n", entry->name);

            // hash_table_add(hash_table, path, entry->hash);

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

void get_all_files_in_directory_recursively(const char *dir_name, char **files, int *index) {
    DIR *d;
    struct dirent *dir;
    d = opendir(dir_name);
    if (d == NULL) {
        return;
    }

    char path[1024];
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_DIR) {
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
                continue;
            snprintf(path, sizeof(path), "%s/%s", dir_name, dir->d_name);
            get_all_files_in_directory_recursively(path, files, index);
        } else {
            snprintf(path, sizeof(path), "%s/%s", dir_name, dir->d_name);
            files[*index] = strdup(path);
            (*index)++;
        }
    }
    closedir(d);
}

char **get_all_files_in_directory() {
    char **files = malloc(sizeof(char *) * 1024);  // Allocate memory for up to 1024 file names
    if (files == NULL) {
        return NULL;
    }

    int index = 0;
    get_all_files_in_directory_recursively(".", files, &index);
    files[index] = NULL;  // Null-terminate the array

    return files;
}

void status(void) {
    printf("Not implemented.\n");
    bool *detached = malloc(sizeof(bool));
    char *head = read_head_file(detached);

    // get to the hash of the head
    printf("head: %s\n", head);
    // const char *PATH = ".git/refs/heads/";

    if (!*detached){
        printf("not detached brodie\n");
    }
    char *hash = malloc(sizeof(char) * (HASH_STRING_LENGTH + 1));
    head_to_hash(head, *detached, hash);

    commit_t *head_commit = read_commit(hash);
    object_hash_t tree_hash;
    memcpy(tree_hash, head_commit->tree_hash, sizeof(object_hash_t));  // Copy the tree_hash

    // map the file names of the commit to their hashes
    hash_table_t *commit_table = hash_table_init();
    expand_tree(tree_hash, commit_table, "");

    // // Read index file
    index_file_t *idx_file = read_index_file();

    list_node_t *curr_node = key_set(idx_file->entries);

    // iterating through the index file

    printf("Staged for commit:\n");

    while (curr_node != NULL){
        char *file_name = curr_node->value;
        index_entry_t *idx_entry = hash_table_get(idx_file->entries, file_name);

        char *hash_commit = (char *) hash_table_get(commit_table, file_name);
        if (hash_commit == NULL) {
            // in the index, not in the commit 
            printf("\tadded: %s\n", file_name);
        } else{
            // The hashes are different, indicating modified
            if (strcmp(idx_entry->sha1, (char *) hash_table_get(commit_table, file_name)) != 0){
                printf("\tmodified: %s\n", file_name);
            }
            // otherwise they're the same file
        }
        curr_node = curr_node->next;
    }

    list_node_t *commit_node = key_set(commit_table);

    // iterating through the commit file
    while (commit_node != NULL){
        char *file_name = commit_node->value;
        // printf("key: %s\n", file_name);

        if (!hash_table_contains(idx_file->entries, file_name)){
            // index file doesn't have it, but commit does
            printf("\tdeleted: %s", file_name);
        }
        commit_node = commit_node->next;
    }

    printf("Not staged for commit:\n");

    // char **files = get_all_files_in_directory();
    // for (int i = 0; files[i] != NULL; i++) {
    //     printf("%s\n", files[i]);
    // }

    // TODO WHY IS THIS UNNECESSARY???
    // free_hash_table(commit_table, free);
    exit(1);
}
