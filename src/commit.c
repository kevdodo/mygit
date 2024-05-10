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

typedef struct directory_file{
    char *file_dir_name;
    bool is_directory;
} directory_file_t;

typedef struct directory{
    /* data */
    bool completed;
    char *name;
    directory_file_t *directory_files;
    size_t num_files;
} directory_t;

directory_t *make_directory(char *name){
    directory_t * directory = malloc(sizeof(directory_t));
    directory->name = malloc(sizeof(char) * (1 + strlen(name)));
    strcpy(directory->name, name);
    directory->num_files = 0;
    directory->directory_files = NULL;
    directory->completed = false;
    return directory;
}

void add_to_directory(directory_t *directory, char * file_name, bool dir){
    directory->directory_files = realloc(directory->directory_files, sizeof(directory_file_t) * (directory->num_files + 1));

    directory_file_t dir_file;
    dir_file.is_directory = dir;
    dir_file.file_dir_name = file_name;
    
    directory->directory_files[directory->num_files] = dir_file;
    directory->num_files++;
}

// void free_directory_file(directory_file_t *directory_file){
//     free(directory_file->file_dir_name);
//     free(directory_file);
// }

void free_directory(directory_t *directory){

    directory_file_t *directory_files = directory->directory_files;
    for (size_t i=0; i < directory->num_files; i++){
        free(directory_files[i].file_dir_name);
    }
    if (directory->directory_files != NULL){
        free(directory->directory_files);
    }
    free(directory->name);
    free(directory);
}

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
    tree->entries = NULL; 
    return tree;
}

void add_to_tree(tree_t *tree, tree_entry_t *tree_entry){
    printf("tree count %lu\n\n", tree->entry_count);
    tree->entries = realloc(tree->entries, (tree->entry_count + 1) * sizeof(tree_entry_t));
    if (tree->entries == NULL) {
        // Handle error
        fprintf(stderr, "Failed to allocate memory.\n");
        return;
    }
    tree->entries[tree->entry_count] = *tree_entry;
    tree->entry_count++;
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



void add_to_directory_map(index_entry_t *index_entry, hash_table_t *directory_map) {
    char *file_path = index_entry->fname;
    // char *file_path_copy = strdup(file_path);
    // char *curr_directory = malloc(1);  
    // *curr_directory = '\0';  
    // char *file = strtok(file_path_copy, "/");

    char **file_split = split_path_into_directories(file_path);

    char *curr_directory = strdup("");  // Initialize curr_directory as an empty string
    
    for (int i = 0; file_split[i] != NULL; i++) {
        if (file_split[i+1] == NULL){
            char * file = file_split[i];
            // printf("Directory: %s\n", curr_directory);
            // printf("curr part: %s\n", file);
            if (hash_table_contains(directory_map, curr_directory)){
                directory_t *directory = hash_table_get(directory_map, curr_directory);
                char* part2 = strdup(file);
                add_to_directory(directory, part2, false);
            } else {
                char* dir_copy = strdup(curr_directory);
                directory_t *directory = make_directory(dir_copy);
                char * bruh = strdup(file);
                add_to_directory(directory, bruh, false);
                hash_table_add(directory_map, dir_copy, directory);
                free(dir_copy);
            }
        } else{

            char *prev_dir = strdup(curr_directory);

            size_t new_length = strlen(curr_directory) + strlen(file_split[i]) + 2;  
            curr_directory = realloc(curr_directory, new_length);  // Reallocate memory for curr_directory

            strcat(curr_directory, file_split[i]);
            strcat(curr_directory, "/");  // Append "/" to curr_directory

            // add to the previous directory if the previous directory
            // exists
            // or make a new directory if it's not in the hash table

            if (hash_table_contains(directory_map, curr_directory)){
                // directory_t *directory = hash_table_get(directory_map, prev_dir);
                // char* part2 = strdup(curr_directory);
                // add_to_directory(directory, part2, true);
            } else {
                char* prev_dir_copy = strdup(prev_dir);
                char* dir_copy = strdup(curr_directory);

                
                printf("prev ddddddddddddir: %s\n", prev_dir);
                printf("curr ddddddddddddir: %s\n", curr_directory);
                printf("next one %s\n", file_split[i+1]);
                assert(hash_table_contains(directory_map, prev_dir_copy));

                directory_t *prev_directory = hash_table_get(directory_map, prev_dir_copy);

                add_to_directory(prev_directory, dir_copy, true);

                directory_t *new_directory = make_directory(dir_copy);

                hash_table_add(directory_map, dir_copy, new_directory);
                // free(dir_copy);
                free(prev_dir_copy);
            }
            free(prev_dir);
        }       
    }
    free(file_split);
    // free(file_path_copy);
    free(curr_directory);
    // while (file != NULL) {
    //     char *next_part = strtok(NULL, "/");
    //     if (next_part == NULL){
    //         printf("Directory: %s\n", curr_directory);
    //         printf("curr part: %s\n", file);
    //         if (hash_table_contains(directory_map, curr_directory)){
    //             directory_t *directory = hash_table_get(directory_map, curr_directory);
    //             char* part2 = strdup(file);

    //             add_to_directory(directory, part2, false);
    //         } else {
    //             char* dir_copy = strdup(curr_directory);
    //             directory_t *directory = make_directory(dir_copy);
    //             char * bruh = strdup(file);
    //             add_to_directory(directory, bruh, false);
    //             hash_table_add(directory_map, dir_copy, directory);
    //             free(dir_copy);
    //         }
    //         break;
    //     }
    //     curr_directory = realloc(curr_directory, strlen(curr_directory) + strlen(file) + 2);
    //     strcat(curr_directory, file);  
    //     printf("Directory: %s\n", curr_directory);
    //     strcat(curr_directory, "/");
    //     file = next_part;
    //     next_part = strtok(NULL, "/");

    // }

}

char *get_last_dir(char *file_dir_name){
    // printf("\tdirectory: %s\n", dir_file.file_dir_name);

    char * full_name = strdup(file_dir_name);     
    char ** full_name_splitted= split_path_into_directories(full_name);
    int count = 0;

    while (full_name_splitted[count] != NULL) {
        count++;
    }
    // printf("count: %d", count)
    char *last_dir;
    if (count != 0){
        last_dir = strdup(full_name_splitted[count-1]);
    } else {
        last_dir = full_name;
    }
    // printf("\t\tlast_dir: %s\n\n", last_dir);

    // for (size_t i=0; full_name_splitted[i] != NULL; i++){
    //     free(full_name_splitted[i]);
    // }
    free(full_name_splitted);
    free(full_name);
    return last_dir;
}

void debug_map(const hash_table_t *dir_map){
    list_node_t *head2 = key_set(dir_map);
    while (head2 != NULL) {
        char *file_path = head2->value;
        directory_t *dir = hash_table_get(dir_map, file_path);
        printf("dir name: %s-------------\n", dir->name);
        for (size_t i=0; i < dir->num_files; i++){
            directory_file_t dir_file = dir->directory_files[i];
            if (dir_file.is_directory){
                printf("\tdirectory: %s\n", dir_file.file_dir_name);

                // get_last_dir(dir_file.file_dir_name);
                
            } else {
                printf("\tnot a directory: %s\n", dir_file.file_dir_name);
            }
        }
        printf("\n");
        head2 = head2->next;
    }

    printf("--------------------   end of debug    ----------------\n\n\n");

    // head2 = key_set(dir_map);

    // while (head2 != NULL) {
    //     char *file_path = head2->value;
    //     directory_t *dir = hash_table_get(dir_map, file_path);
    //     printf("dir name: %s-------------\n", dir->name);
    //     for (size_t i=0; i < dir->num_files; i++){
    //         directory_file_t dir_file = dir->directory_files[i];
    //         if (dir_file.is_directory){
    //             printf("\tdirectory: %s\n", dir_file.file_dir_name);

    //             get_last_dir(dir_file.file_dir_name);
    //         } else {
    //             printf("\tnot a directory: %s\n", dir_file.file_dir_name);
    //         }
    //     }
    //     printf("\n");
    //     head2 = head2->next;
    // }
}


char *get_full_name(char *dir_name, char *file_name){
    char * full_path = malloc(sizeof(char) * (strlen(dir_name) + strlen(file_name) + 1));
    strcpy(full_path, dir_name);
    strcat(full_path, file_name);
    return full_path;
}


char *add_a_stupid_slash(char *string){
    char * name_w_slash = malloc(sizeof(char) * (strlen(string) + 2));
    strcpy(name_w_slash, string);
    strcat(name_w_slash, "/");
    return name_w_slash;
}
bool can_hash(directory_t *dir, hash_table_t *dir_map){
    if (dir->completed){
        return true;
    }
    for (size_t i=0; i < dir->num_files; i++){
        directory_file_t dir_file = dir->directory_files[i];
        if (dir_file.is_directory){
            char *name_w_slash;
            if (dir_file.file_dir_name[strlen(dir_file.file_dir_name)-1] != '/'){
                printf("bruhhhh\n");
                printf("   %c ???????", dir_file.file_dir_name[strlen(dir_file.file_dir_name)-1]);
                name_w_slash = add_a_stupid_slash(dir_file.file_dir_name);
            } else {
                name_w_slash = strdup(dir_file.file_dir_name);
            }
            // char * name_w_slash = add_a_stupid_slash(dir_file.file_dir_name);
            
            printf("name_w_slash: %s\n", name_w_slash);
            directory_t * the_dir = hash_table_get(dir_map, name_w_slash);
            assert(the_dir != NULL);
            free(name_w_slash);

            if (!the_dir->completed){
                return false;
            }
        }
    }
    dir->completed = true;
    return true;
}


void write_tree(FILE *f, directory_t *directory, const hash_table_t * index_table, hash_table_t *tree_map) {
    
    char *contents = malloc(1);
    

    *contents = '\0';
    for (size_t i=0; i < directory->num_files; i ++){
        directory_file_t dir_file = directory->directory_files[i];

        if (dir_file.is_directory){
            // object_hash_t tree_hash = hash_table_get(tree_map, dir_file.file_dir_name);
            // // fwrite(f, )
            // // uint8_t hash_bytes[HASH_BYTES];
            // // hex_to_hash(tree_hash, hash_bytes);
            // contents = realloc(contents, sizeof(contents) + sizeof(MODE_DIRECTORY) + sizeof(char)*(strlen(dir_file.file_dir_name) + 1 + HASH_BYTES));
            // strcat(contents, "10064");
            // strcat(contents, dir_file.file_dir_name);
            // strcat(contents, tree_hash);
            // printf("contents: %s", contents);
        } else {
            char *full_name = get_full_name(directory->name, dir_file.file_dir_name);
            printf("full name : %s\n", full_name);
            index_entry_t *idx_entry = hash_table_get(index_table, full_name);
            assert(idx_entry != NULL);
            // fwrite(f, )
            uint8_t hash_bytes[HASH_BYTES];
            hex_to_hash(idx_entry->sha1, hash_bytes);
            size_t total_size = strlen(contents) + strlen("10064 ") + strlen(dir_file.file_dir_name) + 2*HASH_BYTES + 3;
            contents = realloc(contents, total_size);

            strcat(contents, "10064 ");
            strcat(contents, dir_file.file_dir_name);
            strcat(contents,"\0");

            for (int i = 0; i < HASH_BYTES; i++) {
                char str[3];
                sprintf(str, "%02x", hash_bytes[i]);
                strcat(contents, str);
            }
            
            // strcat(contents, hash_bytes);
            printf("contents: \n%s\n", contents);
            free(full_name);
        }
    }
    size_t len = strlen(contents);
    object_hash_t hash;
    write_object(TREE, contents, strlen(contents), hash);
    // if (fwrite(contents, sizeof(char), len, f) != len) {
    //     fprintf(stderr, "Error writing to file\n");
    // }
    printf("hash %s\n", (char *)hash);
    free(contents);
}






void directory_traversal(FILE *f, directory_t *curr_root_directory, const hash_table_t *dir_map, const hash_table_t * index_table){


    printf("------------------dir from function-------------------\n\n");
    printf("-----------name %s---------------\n", curr_root_directory->name);

    if (can_hash(curr_root_directory, dir_map) ){
        char *dir_name = get_last_dir(curr_root_directory->name);

        printf("i can be hashed %s", dir_name);
        // hash the stuff and return remember to write name of the tree not the
        // name of the 

        // write_tree()
        free(dir_name);
        return;
    } else {
        printf("i can't be hashed %s\n", curr_root_directory->name);
    }
    for (size_t i=0; i < curr_root_directory->num_files; i ++){
        directory_file_t dir_file = curr_root_directory->directory_files[i];
        char *file_name = dir_file.file_dir_name;
        printf("file name %s\n", dir_file.file_dir_name);

        if (dir_file.is_directory){
            printf("dir name: %s\n", dir_file.file_dir_name);
            char *name_w_slash;
            if (dir_file.file_dir_name[strlen(dir_file.file_dir_name)-1] != '/'){
                printf("bruhhhh\n");

                printf("   %c ???????", dir_file.file_dir_name[strlen(dir_file.file_dir_name)-1]);
                name_w_slash = add_a_stupid_slash(dir_file.file_dir_name);
            } else {
                name_w_slash = strdup(dir_file.file_dir_name);
            }

            printf("naem w slash %s\n", name_w_slash);

            assert(hash_table_contains(dir_map, name_w_slash));

            directory_t * directory = hash_table_get(dir_map, name_w_slash);
            if (!directory->completed){
                directory_traversal(f, directory, dir_map, index_table);
                directory->completed = true;
            } else{
                printf("yuhhhhh");
            }
            free(name_w_slash);
        } else {
            // it has to be a blob object, make it into the tree entry
            char *full_path = get_full_name(curr_root_directory->name, file_name);
            // 
            printf("full path: %s\n", full_path);
            assert(hash_table_contains(index_table, full_path));
            free(full_path);
        }
    }
    curr_root_directory->completed = true;
    printf("------------------end of function-------------------\n");
}

void make_tree_from_idx(){
    index_file_t *index_file = read_index_file();
    hash_table_t *index_table = index_file->entries;

    hash_table_sort(index_table);

    list_node_t *head = key_set(index_table);

    // // Count the number of nodes in the linked list
    list_node_t *current = head;

    hash_table_t *dir_map = hash_table_init();


    directory_t *root_dir = make_directory("");


    hash_table_add(dir_map, "", root_dir);

    while (current != NULL) {
        char *file_path = current->value;
        index_entry_t *idx_entry = hash_table_get(index_table, file_path);
        
        char *str = strdup(file_path);
        add_to_directory_map(idx_entry, dir_map);

        // iterate_directories(idx_entry, dir_map);
        current = current->next;
        free(str);
    }

    debug_map((const hash_table_t *)dir_map);

    directory_t *dir_bruhh = hash_table_get(dir_map, "");

    // printf("---------------dir from root------------------\n");
    // for (size_t i=0; i < root_dir->num_files; i++){
    //     directory_file_t dir_file = root_dir->directory_files[i];
    //     if (dir_file.is_directory){
    //         printf("\tdirectory: %s\n", dir_file.file_dir_name);
    //         get_last_dir(dir_file.file_dir_name);
    //     } else {
    //         printf("\tnot a directory: %s\n", dir_file.file_dir_name);
    //     }
    // }



    directory_traversal(NULL, root_dir, dir_map, index_file->entries);

    directory_t *brodie4 = hash_table_get(dir_map, "src/brodie2/brodie3/brodie4/");

    hash_table_t *tree_map = hash_table_init();
    FILE *obj = fopen("uhhhh", "w");
    write_tree(obj, brodie4, index_table, tree_map);

    free_hash_table(tree_map, NULL);

    free_hash_table(dir_map, (free_func_t) free_directory);

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
