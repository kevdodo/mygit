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


typedef struct tree_node {
    char *name;
    struct tree_node **children;
    size_t child_count;
} tree_node_t;

tree_node_t *create_tree_node(char *name) {
    tree_node_t *node = malloc(sizeof(tree_node_t));
    if (node == NULL) {
        return NULL;
    }
    node->name = name;
    node->children = NULL;
    node->child_count = 0;
    return node;
}

int add_child(tree_node_t *parent, tree_node_t *child) {
    parent->children = realloc(parent->children, sizeof(tree_node_t *) * (parent->child_count + 1));
    if (parent->children == NULL) {
        return -1;
    }
    parent->children[parent->child_count] = child;
    parent->child_count++;
    return 0;
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
    hash_table_t *stuff = hash_table_init();

    while (current != NULL) {
        char *file_path = current->value;
        index_entry_t *idx_entry = hash_table_get(index_table, file_path);
        char *pch = strtok(file_path, "/");


        hash_table_add(stuff, pch, file_path);
        current = current->next;
    }


    list_node_t *tree_head = key_set(stuff);
    while (tree_head != NULL) {
        char *file_path = tree_head->value;
        printf("tree paths lol %s\n", file_path);
        tree_head = tree_head->next;
    }

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
