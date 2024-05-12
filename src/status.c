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
    // TODO: make not recursive cuz maybe they have a stack overflow
    tree_t *tree = read_tree(tree_hash);  // Call the read_tree function

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
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0){
                continue;
            }
            snprintf(path, sizeof(path), "%s/%s", dir_name, dir->d_name);

            if (starts_with(path, "./.git") == 1) {
                continue;
            }
            // printf("path %s\n", path);

            get_all_files_in_directory_recursively(path, files, index);
        } else {
            snprintf(path, sizeof(path), "%s/%s", dir_name, dir->d_name);
            files[*index] = strdup(path+2);
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
    bool *detached = malloc(sizeof(bool));
    char *head = read_head_file(detached);

    // get to the hash of the head
    // const char *PATH = ".git/refs/heads/";

    // if (!*detached){
    //     printf("not detached brodie\n");
    // }
    char *hash = malloc(sizeof(char) * (HASH_STRING_LENGTH + 1));
    bool found = head_to_hash(head, *detached, hash);
    hash_table_t *commit_table = hash_table_init();

    if (!found){
        free(hash);
        // todo: worry about the memory leaks

    } else {
        commit_t *head_commit = read_commit(hash);
        object_hash_t tree_hash;
        memcpy(tree_hash, head_commit->tree_hash, sizeof(object_hash_t));  // Copy the tree_hash

        // map the file names of the commit to their hashes
        expand_tree(tree_hash, commit_table, "");
    }

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
                printf("\tnew file: %s\n", file_name);
            } else{
                // The hashes are different, indicating modified
                if (strcmp(idx_entry->sha1, hash_commit) != 0){
                    printf("\tmodified: %s\n", file_name);
                }
                // otherwise they're the same file
            }
            curr_node = curr_node->next;
        }

        list_node_t *commit_node = key_set(commit_table);

        // iterating through the commit file to track delted files
        while (commit_node != NULL){
            char *file_name = commit_node->value;

            if (!hash_table_contains(idx_file->entries, file_name)){
                // index file doesn't have it, but commit does
                printf("\tdeleted: %s\n", file_name);
            }
            commit_node = commit_node->next;
        }
    
    char **files = get_all_files_in_directory();

    hash_table_t *work_tree = hash_table_init();

    printf("Not staged for commit:\n");
    for (int i = 0; files[i] != NULL; i++) {
        // printf("%s\n", files[i]);
        char *file_path = files[i];
        char *file_contents = get_file_contents(file_path);

        object_hash_t hash;
        get_object_hash(BLOB, file_contents, strlen(file_contents), hash);

        hash_table_add(work_tree, file_path, (char *) hash);
        if (hash_table_contains(idx_file->entries, file_path)){
            // printf("yuhhhh\n");
            index_entry_t *index_entry = hash_table_get(idx_file->entries, file_path);
            if (strcmp(hash, index_entry->sha1) != 0){
                // printf("idx_hash  %s\n", index_entry->sha1);
                // printf("file hash %s\n",  hash);
                printf("\tmodified: %s\n", file_path);
            }
        }
        free(file_contents);
    }


    list_node_t *curr_node2 = key_set(idx_file->entries);

    // iterating through the index file to see if one has gotten deleted
    while (curr_node2 != NULL){
        char *file_name = curr_node2->value;
        if (!hash_table_contains(work_tree, file_name)){
            printf("\tdeleted: %s\n", file_name);
        }
        curr_node2 = curr_node2->next;
    }

    printf("Untracked files:\n");
    for (int i = 0; files[i] != NULL; i++) {
        char *file_path = files[i];
        if (!hash_table_contains(idx_file->entries, file_path)){
            printf("\t%s\n", file_path);
        }
    }

    // TODO WHY IS THIS UNNECESSARY???
    free(detached);
    free(head);
    free(hash);
    free_index_file(idx_file);
    free_hash_table(work_tree, NULL);
    // free_hash_table(commit_table, free);
}
