#include "status.h"
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "index_io.h"
#include "object_io.h"
#include "ref_io.h"


#include "util.h"

struct list_node {
   list_node_t *next;
   void *value;
};
struct linked_list {
    list_node_t *head;
    list_node_t **tail;
};


void status(void) {
    printf("Not implemented.\n");


    bool *detached = malloc(sizeof(bool));
    char *head = read_head_file(detached);

    // get to the hash of the head
    printf("head: %s\n", head);
    // const char *PATH = ".git/refs/heads/";




    if (!*detached){
        printf("detached brodie\n");
    }
    char *hash = malloc(sizeof(char) * (HASH_STRING_LENGTH + 1));
    head_to_hash(head, *detached, hash);

    commit_t *head_commit = read_commit(hash);
    object_hash_t tree_hash;
    memcpy(tree_hash, head_commit->tree_hash, sizeof(object_hash_t));  // Copy the tree_hash

    tree_t *tree = read_tree(tree_hash);  // Call the read_tree function

    for (int i = 0; i < tree->entry_count; i++) {
        tree_entry_t *entry = &tree->entries[i];
        printf("file name: %s\n", entry->name);
        // Now you can access the fields of entry
    }

    // printf("hash %s\n", hash);

    // // first two characters is the directory of the object

    // char first_two[3];  // Array to hold the first two characters and the null terminator
    // strncpy(first_two, hash, 2);  // Copy the first two characters from hash
    // first_two[2] = '\0';  // Add the null terminator
    // printf("first two %s\n", first_two);

    // int hash_length = strlen(hash);

    // char last_38[hash_length - 1];  // Array to hold the last 38 characters and the null terminator
    // strncpy(last_38, &hash[hash_length - 38], 38);  // Copy the last 38 characters from hash
    // last_38[38] = '\0';  // Add the null terminator

    // printf("last 38: %s\n", last_38);

    // const char *OBJECT_DIRECTORY = ".git/objects/";

    // // // Concatenate PATH and head to form the full path
    // char *head_obj_path = malloc(sizeof(char) * (strlen(OBJECT_DIRECTORY) + strlen(first_two) + strlen(last_38) + 3)); // +3 for the null terminator and two '/' characters

    // if (head_obj_path == NULL) {
    //     perror("Failed to allocate memory for head_obj_path");
    //     return;
    // }

    // strcpy(head_obj_path, OBJECT_DIRECTORY);
    // strcat(head_obj_path, first_two);
    // strcat(head_obj_path, "/");
    // strcat(head_obj_path, last_38);

    // printf("head_obj path: %s\n", head_obj_path);


    // Open the file
    // FILE *file = fopen(full_path, "r");
    // if (file == NULL) {
    //     perror("Failed to open file");
    //     free(full_path);
    //     return;
    // }

    // // Get the file contents
    // char *file_contents = get_file_contents(full_path);
    // if (file_contents == NULL) {
    //     perror("Failed to get file contents");
    //     fclose(file);
    //     free(full_path);
    //     return;
    // }

    // printf("File contents: %s\n", file_contents);

    // Clean up
    // free(file_contents);
    // fclose(file);
    // free(full_path);

    // free(detached);

    // index_file_t * index_file = read_index_file();

    // hash_table_t *index_table = index_file->entries;
    // // uint32_t index_cnts = get_idx_file_cnts(file_paths, file_count, index_table);
    // // printf("index_cnts: %u\n", index_cnts);
    // printf("Staged for commit:\n");
    // list_node_t *current_node = key_set(index_table);
    // while (current_node != NULL) {
    //     const char *key = current_node->value;
    //     // printf("current node: %s\n", key);
    //     index_entry_t *index_entry = hash_table_get(index_table, key);
    //     printf("fname %s\n", index_entry->fname);
    //     current_node = current_node->next;
    // }

    // printf("Not staged for commit:\n");
    // printf("Untracked Files\n");
    exit(1);
}
