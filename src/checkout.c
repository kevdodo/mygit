// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <ctype.h>
// #include "checkout.h"
// #include "ref_io.h"
// #include "object_io.h"
// #include "util.h"
// #include "constants.h"
// #include "index_io.h"
// #include "status.h"
// #include <sys/stat.h>
// #include "add.h"
// #include <unistd.h>

// #include "hash_table.h"

// struct list_node {
//    list_node_t *next;
//    void *value;
// };
// struct linked_list {
//     list_node_t *head;
//     list_node_t **tail;
// };

// char *get_head_commit_hash(){
//     bool detached;    
//     char *head = read_head_file(&detached);
//     char *commit_hash = malloc(sizeof(char) * (HASH_STRING_LENGTH + 1));
//     // object_hash_t commit_hash;
//     bool found = head_to_hash(head, detached, commit_hash);
//     if (!found){
//         free(commit_hash);
//         printf("Hash not found\n");
//         commit_hash = NULL;
//     }
//     free(head);
//     return commit_hash;
// }


// hash_table_t *get_curr_table(){    
//     char *curr_head_commit = get_head_commit_hash();

//     if (curr_head_commit == NULL){
//         printf("head commit isn't real\n");
//         exit(1);
//     }

//     commit_t *commit = read_commit(curr_head_commit);

//     hash_table_t *curr_hash_table = hash_table_init();

//     expand_tree(commit->tree_hash, curr_hash_table, "");

//     free(curr_head_commit);
//     free_commit(commit);
//     return curr_hash_table;
// }

// bool is_valid_commit_hash(const char *hash) {
//     // Check if the hash is 40 characters long
//     if (strlen(hash) != 40) {
//         return false;
//     }

//     // Check if all characters are hexadecimal
//     for (int i = 0; i < 40; i++) {
//         if (!isxdigit(hash[i])) {
//             return false;
//         }
//     }

//     return true;
// }

// bool check_local_change(char *file_name, index_entry_t *idx_entry){
//     struct stat file_stat;
//     if (stat(file_name, &file_stat) == 0) {
//         time_t file_mtime = file_stat.st_mtime;
//         if (file_mtime == idx_entry->mtime) {
//             // mtime of the file is the same as idx_entry->mtime
//         } else {
//             // mtime of the file is different from idx_entry->mtime
//             return true;
//         }
//     } else {
//         // handle error from stat, file might not exist which is ok
//     }
//     return false;
// }


// void checkout(const char *checkout_name, bool make_branch) {
//     if (make_branch) {
//         // Create a new branch pointing to the current value of HEAD
//         char *head_commit_hash = get_head_commit_hash();

//         printf("checkout name: %s\n", checkout_name);

//         if (head_commit_hash == NULL){
//             printf("head commit isn't real\n");
//             return;
//         }

//         set_branch_ref(checkout_name, head_commit_hash);

//         write_head_file(checkout_name, false);

//         free(head_commit_hash);
//         return;
//     } 

//     char **changed_files = NULL;
//     int changed_files_count = 0;
    
//     bool detached;    
//     char *curr_head = read_head_file(&detached);

//     char *head_commit = get_head_commit_hash();
    
//     if (head_commit == NULL){
//         printf("Current head isn't real\n");
//         return; 
//     }
    
//     hash_table_t *curr_commit_table = get_curr_table();


//     index_file_t *idx_file = read_index_file();
//     list_node_t *temp_commit_node = key_set(curr_commit_table);
//     object_hash_t hash;
//     if (!get_branch_ref(checkout_name, hash)){
//         printf("Not a branch name\n");
//         return;
//     }

//     if (is_valid_commit_hash(checkout_name)) {
//         // If name_or_hash is a valid commit hash, checkout to that commit
//         write_head_file(checkout_name, true);
//     } else {
//         // Otherwise, assume name_or_hash is a branch name and checkout to that branch
//         write_head_file(checkout_name, false);
//         //
//     }
    
//     hash_table_t *new_commit_table = get_curr_table();

//     list_node_t *new_commit_node = key_set(new_commit_table);

//     while (new_commit_node != NULL){
//         char *file_name = new_commit_node->value;
//         char *new_hash = hash_table_get(new_commit_table, file_name);
//         char *curr_hash = hash_table_get(curr_commit_table, file_name);
//         printf("curr_file_name: %s\n", file_name);
//         // the files are different
//         if (curr_hash == NULL || strcmp(curr_hash, new_hash) != 0){
//             index_entry_t * idx_entry = hash_table_get(idx_file->entries, file_name); 
//             printf("new hash: %s\n\n\n", new_hash);

//             if (access(file_name, F_OK) != -1){
//                 bool local_change = check_local_change(file_name, idx_entry);
//                 if (local_change){
//                     printf("%s has unstaged changes\n", file_name);
//                     write_head_file(curr_head, detached);
//                     exit(1);
//                 }

//                 if (strcmp(curr_hash, idx_entry->sha1)){
//                     printf("%s has uncomomited changes\n", file_name);
//                     write_head_file(curr_head, detached);
//                     exit(1);
//                 }
//             }
            

//             object_type_t obj_type;
//             size_t length;
//             char *blob = read_object(new_hash, &obj_type, &length);
//             if (blob == NULL) {
//                 printf("Failed to open object: %s\n", new_hash);
//                 exit(1);
//             }

//             // Open the file in write mode
//             FILE *file = fopen(file_name, "w");
//             if (file == NULL) {
//                 printf("Failed to open file: %s\n", file_name);
//                 exit(1);
//             }

//             changed_files = realloc(changed_files, (changed_files_count + 1) * sizeof(char *));
//             changed_files[changed_files_count] = strdup(file_name);
//             changed_files_count++;

//             // Write the blob contents to the file
//             fwrite(blob, sizeof(char), length, file);

//             // Close the file
//             fclose(file);

//             free(blob);
//         }
//         new_commit_node = new_commit_node->next;
//     }

//     // delete files if they don't exist
//     list_node_t *curr_commit_keys = key_set(curr_commit_table);

//     while (curr_commit_keys != NULL) {
//         char *file_name = curr_commit_keys->value;
//         char *new_hash = hash_table_get(new_commit_table, file_name);

//         // If the file doesn't exist in the new commit, delete it
//         if (new_hash == NULL) {
//             if (remove(file_name) == 0) {
//                 printf("Deleted file: %s\n", file_name);
//                 changed_files = realloc(changed_files, (changed_files_count + 1) * sizeof(char *));
//                 changed_files[changed_files_count] = strdup(file_name);
//                 changed_files_count++;
//             } else {
//                 printf("Failed to delete file: %s\n", file_name);
//             }
//         }

//         curr_commit_keys = curr_commit_keys->next;
//     }


//     // update the index file
//     add_files(changed_files, changed_files_count);

//     for (int i = 0; i < changed_files_count; i++) {
//         free(changed_files[i]);
//     }

//     free_index_file(idx_file);
//     free(curr_head);
//     free(changed_files);
//     free_hash_table(curr_commit_table, free);
//     free_hash_table(new_commit_table, free);
//     free(head_commit);
//     printf("checkout completed\n");
// }


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "checkout.h"
#include "ref_io.h"
#include "object_io.h"
#include "index_io.h"
// const char BRANCH_REFS_DIR[] = ".git/refs/heads/";


bool create_branch(const char *filename, const char *contents) {
    // Calculate the total length of the file path
    size_t path_length = strlen(".git/refs/heads/") + strlen(filename) + 2; // +1 for the null terminator
    
    // Allocate memory for the full file path
    char *full_path = (char *)malloc(path_length);
    if (full_path == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return 0;
    }
    
    // Concatenate the directory path and filename
    snprintf(full_path, path_length, "%s%s", ".git/refs/heads/", filename);
    
    if (access(full_path) !=1){
        printf("fatal: A branch named '%s' already exists.\n", filename);
        free(full_path);
        return 0;
    }

    // Open the file in write mode
    FILE *file = fopen(full_path, "w");
    if (file == NULL) {
        fprintf(stderr, "Error: Unable to open file %s\n", full_path);
        free(full_path); // Free the allocated memory
        return 0;
    }
    
    // Write the contents to the file
    if (fprintf(file, "%s\n", contents) < 0) {
        fprintf(stderr, "Error: Unable to write contents to file %s\n", full_path);
    }
    
    // Close the file and free the allocated memory
    fclose(file);
    free(full_path);
    return 1;
}


bool is_commit_hash(const char *str) {
    // Check if the string is exactly 40 characters long
    if (strlen(str) != 40) {
        return false;
    }

    // Check each character of the string
    for (int i = 0; i < 40; i++) {
        char c = str[i];
        // Check if the character is a valid hexadecimal digit
        if (!isxdigit(c)) {
            return false;
        }
    }

    // If all characters are valid hexadecimal digits, it's a commit hash
    return true;
}

void checkout(const char *checkout_name, bool make_branch) {
    // Check if the "make_branch" flag is set
    if (make_branch) {
        bool *detatched = malloc(sizeof(bool));
        char *head_file_contents = read_head_file(detatched);
        object_hash_t head_hash;
        bool commit_exists = head_to_hash(head_file_contents, *detatched, head_hash); 
        if (commit_exists){
            if(!create_branch(checkout_name, head_hash)){
                free(detatched);
                free(head_file_contents);
                return;
            };
        }

        write_head_file(checkout_name, false);
        printf("Switched to a new branch '%s'\n", checkout_name);


        free(detatched);
        free(head_file_contents);
    } else {
        // Update HEAD to the checkout name (ref or commit hash)
        // if there are any staged changes raise an error

        index_file_t *idx_file = read_index_file();



        if (is_commit_hash(checkout_name)){
            // if commit hash
            commit_t *commit = read_commit(checkout_name); // checks if hash is a valid commit hash (kind of jank, I know)
            write_head_file(checkout_name, true);
            free_commit(commit);
        } else {
            // if ref
            FILE *b = open_branch_ref(checkout_name, "r");
            if (b == NULL){
                printf("error: pathspec '%s' did not match any file(s) known to mygit\n", checkout_name);
                return;
            }
            fclose(b);

            write_head_file(checkout_name, false);
        }
        // Update the work tree and index accordingly
        // Print a message indicating that HEAD has been updated

    }
}