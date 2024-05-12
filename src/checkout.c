#include <stdio.h>
#include <stdlib.h>
#include "checkout.h"
#include "ref_io.h"
#include "object_io.h"
#include "util.h"
#include "constants.h"
#include "index_io.h"

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
        // if (update_head_to_ref_or_commit(checkout_name) != 0) {
        //     fprintf(stderr, "Failed to update HEAD to ref or commit: %s\n", checkout_name);
        //     exit(1);
        // }


        index_file_t *idx_file = read_index_file();

        list_node_t *curr_node = key_set(idx_file->entries);

        // iterating through the index file

        printf("Staged for commit:\n");

        while (curr_node != NULL){
            char *file_name = curr_node->value;
            index_entry_t *idx_entry = hash_table_get(idx_file->entries, file_name);
            }
            curr_node = curr_node->next;
        }

    //     // Update the work tree and index
    //     if (update_worktree_and_index() != 0) {
    //         fprintf(stderr, "Failed to update work tree and index\n");
    //         exit(1);
    //     }
    // }

    // // Check for unstaged modifications that are identical in the old HEAD and the new HEAD
    // if (has_unstaged_modifications()) {
    //     fprintf(stderr, "There are unstaged modifications. Please commit or stash them before you switch branches.\n");
    //     exit(1);
    }
}