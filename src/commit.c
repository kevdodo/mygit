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

// dictionary from directories to the tree objects
tree_entry_t *make_tree_entry(char *name, file_mode_t mode, object_hash_t hash){
    tree_entry_t *tree_entry = malloc(sizeof(tree_entry_t));
    tree_entry->mode = mode;
    strcpy(tree_entry->hash, hash);
    tree_entry->name = name;
    return tree_entry;
}

// starts off with one tree entry
tree_t *make_empty_tree(){

    
    tree_t *tree = malloc(sizeof(tree_t));
    assert(tree != NULL);
    tree->entry_count = 0;
    tree->entries = NULL; //malloc(sizeof(tree_entry_t));
    // tree_t * start_tree = malloc(sizeof(*start_tree));
    // //malloc(sizeof(tree_entry_t));
    // // start_tree->entries[0] = tree_entry;
    // start_tree->entry_count = 1;
    // start_tree->entries = NULL;
    return tree;
}

void add_to_tree(tree_t *tree, tree_entry_t *tree_entry){

    printf("tree count %lu\n\n", tree->entry_count);
    // printf("size of entry %lu", sizeof(tree_entry_t));

    tree->entries = realloc(tree->entries, (tree->entry_count + 1) * sizeof(tree_entry_t));
    if (tree->entries == NULL) {
        // Handle error
        fprintf(stderr, "Failed to allocate memory.\n");
        return;
    }
    tree->entries[tree->entry_count] = *tree_entry;
    tree->entry_count++;

    // free_tree_entry(tree_entry);
}


// void add_tree(tree_t *tree, char*last_dir, index_entry_t *index_entry){
//     for (size_t i = 0; i < tree->entry_count; i++) {
//         if (strcmp(tree->entries[i].name, last_dir) == 0) {
//             // Found a match
//             printf("Found a match: %s\n", tree->entries[i].name);
//             return;
//         }
//     }
//     // No match found, add to the tree
//     printf("No match found for: %s\n", last_dir);

//     // add_to_tree(tree, )
// }
//     typedef struct {
//     file_mode_t mode;
//     char *name; // file or directory name
//     object_hash_t hash; // blob (for files) or tree (for directories)
// } tree_entry_t;


void iterate_directories(index_entry_t *index_entry, hash_table_t *directory_map) {
    char *file_path = index_entry->fname;
    char *file_path_copy = strdup(file_path);
    char *curr_directory = malloc(1);  
    *curr_directory = '\0';  
    char *part = strtok(file_path_copy, "/");

    while (part != NULL) {
        char *next_part = strtok(NULL, "/");

        if (next_part == NULL){
            printf("Directory: %s\n", curr_directory);
            printf("curr part: %s\n", part);
            if (hash_table_contains(directory_map, curr_directory)){
                tree_t *tree = hash_table_get(directory_map, curr_directory);
                tree_entry_t *entry = (file_path, MODE_FILE, index_entry->sha1);
                add_to_tree(tree, entry);
            } else {
                tree_t *tree = make_empty_tree();
                char* dir_copy = strdup(curr_directory);
                tree_entry_t *entry = make_tree_entry(dir_copy, MODE_FILE, index_entry->sha1);
                add_to_tree(tree, entry);
                hash_table_add(directory_map, dir_copy, tree);
                free_tree_entry(entry);
            }
            break;
        }

        
        curr_directory = realloc(curr_directory, strlen(curr_directory) + strlen(part) + 2);
        strcat(curr_directory, part);  
        printf("Directory: %s\n", curr_directory);
        strcat(curr_directory, "/");
        // prev_part = realloc(prev_part, strlen(part) + 1);
        // prev_part = part;
        part = next_part;
        next_part = strtok(NULL, "/");

    }

    free(file_path_copy);
    free(curr_directory);
}




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

    tree_t *root_tree = make_empty_tree();

    char *prev_dir = "";
    hash_table_t *dir_tree_map = hash_table_init();

    hash_table_add(dir_tree_map, "", root_tree);


    while (current != NULL) {
        char *file_path = current->value;
        index_entry_t *idx_entry = hash_table_get(index_table, file_path);
        
        char *str = strdup(file_path);
        printf("full file: %s\n", str);

        iterate_directories(idx_entry, dir_tree_map);

        current = current->next;
        free(str);
    }


    // list_node_t *tree_head = key_set(stuff);
    // while (tree_head != NULL) {
    //     char *file_path = tree_head->value;
    //     printf("tree paths lol %s\n", file_path);
    //     tree_head = tree_head->next;
    // }

    free_hash_table(dir_tree_map, free_tree_mine);
    free_index_file(index_file);

    // free_hash_table(dir_tree_map, free_tree);

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

    // printf("Not implemented.\n");
    exit(1);
}
