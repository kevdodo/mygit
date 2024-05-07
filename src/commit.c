#include "commit.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "config_io.h"
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


// typedef struct tree_node {
//     char *name;
//     struct tree_node **children;
//     size_t child_count;
// } tree_node_t;

// tree_node_t *create_tree_node(char *name) {
//     tree_node_t *node = malloc(sizeof(tree_node_t));
//     if (node == NULL) {
//         return NULL;
//     }
//     node->name = name;
//     node->children = NULL;
//     node->child_count = 0;
//     return node;
// }

// int add_child(tree_node_t *parent, tree_node_t *child) {
//     parent->children = realloc(parent->children, sizeof(tree_node_t *) * (parent->child_count + 1));
//     if (parent->children == NULL) {
//         return -1;
//     }
//     parent->children[parent->child_count] = child;
//     parent->child_count++;
//     return 0;
// }



// dictionary from directories to the tree objects
tree_entry_t *make_tree_entry(char *name, file_mode_t mode, object_hash_t hash){
    tree_entry_t *tree_entry = malloc(sizeof(tree_entry_t));
    tree_entry->mode = mode;
    strcpy(tree_entry->hash, hash);
    tree_entry->name = name;
}

// starts off with one tree entry
tree_t *make_empty_tree(){
    tree_t * start_tree = malloc(sizeof(tree_t));

    //malloc(sizeof(tree_entry_t));
    // start_tree->entries[0] = tree_entry;
    start_tree->entry_count = 0;
    start_tree->entries = NULL;
    return start_tree;
}

void add_to_tree(tree_t *tree, tree_entry_t *tree_entry){
    tree->entries = realloc(tree->entries, (tree->entry_count + 1) * sizeof(tree_entry_t));
    if (tree->entries == NULL) {
        // Handle error
        fprintf(stderr, "Failed to allocate memory.\n");
        return;
    }
    tree->entries[tree->entry_count] = *tree_entry;
    tree->entry_count++;
}


void add_tree(tree_t *tree, char*last_dir, index_entry_t *index_entry){
    for (int i = 0; i < tree->entry_count; i++) {
        if (strcmp(tree->entries[i].name, last_dir) == 0) {
            // Found a match
            printf("Found a match: %s\n", tree->entries[i].name);
            return;
        }
    }
    // No match found, add to the tree
    printf("No match found for: %s\n", last_dir);

    // add_to_tree(tree, )
}
//     typedef struct {
//     file_mode_t mode;
//     char *name; // file or directory name
//     object_hash_t hash; // blob (for files) or tree (for directories)
// } tree_entry_t;


void make_tree_from_idx(){
    index_file_t *index_file = read_index_file();


    hash_table_t *index_table = index_file->entries;

    hash_table_sort(index_table);

    list_node_t *head = key_set(index_table);

    // // Count the number of nodes in the linked list
    // size_t count = 0;
    list_node_t *current = head;

    // maps file_names to file names, and directories to all their shit


    // directories -> tree directories
    hash_table_t *dir_tree_map = hash_table_init();

    tree_t *root_tree = make_empty_tree();

    char *prev_dir = "";

    hash_table_add(dir_tree_map, "", root_tree);


    while (current != NULL) {
        char *file_path = current->value;
        index_entry_t *idx_entry = hash_table_get(index_table, file_path);

        
        char *str = strdup(file_path);
        printf("full file: %s\n", str);

        char *pch = strtok (str,"/");

        char *last_pch = "";
        
        // char *curr_dir_path = malloc(1);  // Start with enough space for the null character
        // *curr_dir_path = '\0';  // Make it an empty string

        // while (pch != NULL) {
        //     printf ("%s\n", pch);

        //     // Resize curr_dir_path and append the current token and a slash
        //     curr_dir_path = realloc(curr_dir_path, strlen(curr_dir_path) + strlen(last_pch) + 2);
        //     strcat(curr_dir_path, last_pch);
        //     if (strcmp(last_pch, "") != 0){
        //         strcat(curr_dir_path, "/");
        //     }

        //     if (!hash_table_get(dir_tree_map, curr_dir_path)){
        //         // want to make a new tree if it hasn't been initialized already
        //         tree_t *new_tree = make_empty_tree();
        //         hash_table_add(dir_tree_map, curr_dir_path, new_tree);
        //     } else {
        //         tree_t *tree = hash_table_get(dir_tree_map, curr_dir_path);
        //         add_tree(tree, last_pch);
        //     }
        //     last_pch = pch;
        //     pch = strtok(NULL, "/");
        // }
        // printf("Current directory path: %s\n", curr_dir_path);
        // hash_table_add(stuff, pch, file_path);
        current = current->next;
    }


    // list_node_t *tree_head = key_set(stuff);
    // while (tree_head != NULL) {
    //     char *file_path = tree_head->value;
    //     printf("tree paths lol %s\n", file_path);
    //     tree_head = tree_head->next;
    // }

    free_index_file(index_file);
    // free_hash_table(index_table, free);

}

commit_t *get_head_commit(){
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
    return head_commit;
}

void commit(const char *commit_message) {

    // index_file_t *read_index_file();
    make_tree_from_idx();

    printf("Not implemented.\n");
    exit(1);
}
