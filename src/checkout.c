#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
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


hash_table_t *get_curr_table(){    
    char *curr_head_commit = get_head_commit_hash();

    if (curr_head_commit == NULL){
        printf("head commit isn't real\n");
        exit(1);
    }

    commit_t *commit = read_commit(curr_head_commit);

    hash_table_t *curr_hash_table = hash_table_init();

    expand_tree(commit->tree_hash, curr_hash_table, "");

    free(curr_head_commit);
    free_commit(commit);
    return curr_hash_table;
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
        // Getting current commit hash_table: 

        char *head_commit = get_head_commit_hash();
        
        if (head_commit == NULL){
            printf("head commit isn't real\n");
            exit(1);
        }
        
        hash_table_t *curr_commit_table = get_curr_table();

        object_hash_t hash;
        if (!get_branch_ref(checkout_name, hash)){
            printf("Not a branch name\n");
            exit(1);
        }

        if (is_valid_commit_hash(checkout_name)) {
            // If name_or_hash is a valid commit hash, checkout to that commit
            write_head_file(checkout_name, true);
        } else {
            // Otherwise, assume name_or_hash is a branch name and checkout to that branch
            write_head_file(checkout_name, false);
        }
        
        hash_table_t *new_commit_table = get_curr_table();



        list_node_t *new_commit_node = key_set(new_commit_table);

        while (new_commit_node != NULL){
            char *file_name = new_commit_node->value;
            char *new_hash = hash_table_get(new_commit_table, file_name);
            char *curr_hash = hash_table_get(curr_commit_table, file_name);
            printf("curr_file_name: %s\n", file_name);
            // the files are different
            if (curr_hash == NULL || strcmp(curr_hash, new_hash) != 0){
                printf("new hash: %s\n", new_hash);

                blob_t *blob = read_blob(new_hash);
                if (blob == NULL) {
                    printf("Failed to open object: %s\n", new_hash);
                    exit(1);
                }
                // printf("blob contents: \n\n %s", blob->contents);
                // Write to the file or do something else with it here
                free_blob(blob);
            }
            new_commit_node = new_commit_node->next;
        }
        free_hash_table(curr_commit_table, free);
        free_hash_table(new_commit_table, free);
        free(head_commit);
        printf("checkout completed\n");
    }
}