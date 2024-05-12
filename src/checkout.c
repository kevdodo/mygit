#include <stdio.h>
#include <stdlib.h>
#include "checkout.h"
#include "ref_io.h"
#include "object_io.h"
#include "util.h"
#include "constants.h"
#include "index_io.h"
#include "status.h"

#include "hash_table.h"

struct list_node {
   list_node_t *next;
   void *value;
};
struct linked_list {
    list_node_t *head;
    list_node_t **tail;
};

char *get_head_commit_hash(){
    bool detached;    
    char *head = read_head_file(&detached);
    char *commit_hash = malloc(sizeof(char) * (HASH_STRING_LENGTH + 1));
    // object_hash_t commit_hash;
    bool found = head_to_hash(head, detached, commit_hash);
    if (!found){
        free(commit_hash);
        printf("Hash not found\n");
        commit_hash = NULL;
    }
    free(head);
    return commit_hash;
}

void expand_tree2(object_hash_t tree_hash, hash_table_t* hash_table, char *curr_chars){
    // TODO: make not recursive cuz maybe they have a stack overflow
    tree_t *tree = read_tree(tree_hash);  // Call the read_tree function

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

void checkout(const char *checkout_name, bool make_branch) {
    if (make_branch) {
        // Create a new branch pointing to the current value of HEAD
        char *head_commit_hash = get_head_commit_hash();

        // printf("head commit: %s\n", head_commit_hash);
        printf("checkout name: %s\n", checkout_name);

        if (head_commit_hash == NULL){
            printf("head commit isn't real\n");
            exit(1);
        }

        set_branch_ref(checkout_name, head_commit_hash);

        bool detached;    
        char *head = read_head_file(&detached);

        write_head_file(checkout_name, detached);

        free(head);

        free(head_commit_hash);
    } else {

        bool detached;    
        char *head = read_head_file(&detached);

        write_head_file(checkout_name, detached);

        char *head_commit = get_head_commit_hash();

        if (head_commit == NULL){
            printf("head commit isn't real\n");
            exit(1);
        }

        commit_t *commit = read_commit(head_commit);

        hash_table_t *hash_table = hash_table_init();

        expand_tree(commit->tree_hash, hash_table, "");

        index_file_t *idx_file = read_index_file();

        list_node_t *curr_node = key_set(idx_file->entries);

        // iterating through the index file
        printf("Staged for commit:\n");

        while (curr_node != NULL){
            char *file_name = curr_node->value;
            index_entry_t *idx_entry = hash_table_get(idx_file->entries, file_name);
            curr_node = curr_node->next;

        }
    }
}