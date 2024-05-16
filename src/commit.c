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
    // printf("tree count %lu\n\n", tree->entry_count);
    tree->entries = realloc(tree->entries, (tree->entry_count + 1) * sizeof(tree_entry_t));
    if (tree->entries == NULL) {
        // Handle error
        // fprintf(stderr, "Failed to allocate memory.\n");
        return;
    }
    tree->entries[tree->entry_count] = *tree_entry;
    tree->entry_count++;
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
                name_w_slash = add_a_stupid_slash(dir_file.file_dir_name);
            } else {
                name_w_slash = strdup(dir_file.file_dir_name);
            }
            // char * name_w_slash = add_a_stupid_slash(dir_file.file_dir_name);
            
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


void write_tree(directory_t *directory, const hash_table_t * index_table, hash_table_t *tree_map) {
    
    uint8_t *contents = malloc(1);
    size_t curr_size = 0;

    // *contents = '\0';
    for (size_t i=0; i < directory->num_files; i ++){
        directory_file_t dir_file = directory->directory_files[i];

        if (dir_file.is_directory){

            assert(hash_table_contains(tree_map, dir_file.file_dir_name));

            void *tree_hash = hash_table_get(tree_map, dir_file.file_dir_name);
            // printf("tree hash len: %s\n", strlen(tree_hash));

            char *last_dir = get_last_dir(dir_file.file_dir_name);

            // char mode_str[12];  // Large enough to hold a 32-bit integer in octal
            // sprintf(mode_str, "%o", idx_entry->mode);
            //bruh

            contents = realloc(contents, curr_size + strlen("40000 ") + strlen(last_dir) + 2 + HASH_BYTES);
            
            memcpy(contents + curr_size, "40000 ", strlen("40000 "));
            curr_size += strlen("40000 ");


            memcpy(contents + curr_size, last_dir, strlen(last_dir));
            curr_size += strlen(last_dir);

            memcpy(contents + curr_size, "\0", 1);
            curr_size += 1;

            uint8_t hash_bytes[HASH_BYTES];
            hex_to_hash(tree_hash, hash_bytes);
            memcpy(contents + curr_size, hash_bytes, HASH_BYTES);
            curr_size += HASH_BYTES;
            free(last_dir);

        } else {
            char *full_name = get_full_name(directory->name, dir_file.file_dir_name);
            // printf("full name : %s\n", full_name);
            index_entry_t *idx_entry = hash_table_get(index_table, full_name);
            assert(idx_entry != NULL);

            char mode_str[12];  // Large enough to hold a 32-bit integer in octal
            sprintf(mode_str, "%o", idx_entry->mode);
            size_t total_size = curr_size + strlen(mode_str) + strlen(dir_file.file_dir_name) + HASH_BYTES + 3;
            contents = realloc(contents, total_size);
            
            
            memcpy(contents + curr_size, mode_str, strlen(mode_str));
            curr_size += strlen(mode_str);
            
            memcpy(contents + curr_size, " ", 1);
            curr_size += 1;
            memcpy(contents + curr_size, dir_file.file_dir_name, strlen(dir_file.file_dir_name));
            curr_size += strlen(dir_file.file_dir_name);
            memcpy(contents + curr_size, "\0", 1);

            curr_size += 1;

            // size_t contents_len = strlen(contents);
            uint8_t hash_bytes[HASH_BYTES];
            hex_to_hash(idx_entry->sha1, hash_bytes);
            memcpy(contents + curr_size, hash_bytes, HASH_BYTES);
            curr_size += HASH_BYTES;
            
            // strcat(contents, hash_bytes);
            // printf("contents: \n%s\n", contents);
            free(full_name);
        }
    }
    object_hash_t hash;
    write_object(TREE, (void *)contents, curr_size, hash);

    char *hash_brodie = malloc(sizeof(char) * 41);
    strcpy(hash_brodie, hash);

    hash_table_add(tree_map, directory->name, hash_brodie);
    free(contents);
}






void directory_traversal(directory_t *curr_root_directory, hash_table_t *dir_map, const hash_table_t * index_table, hash_table_t * tree_map){



    if (can_hash(curr_root_directory, dir_map) ){
        char *dir_name = get_last_dir(curr_root_directory->name);

        // hash the stuff and return remember to write name of the tree not the
        // name of the 
        write_tree(curr_root_directory, index_table, tree_map);
        free(dir_name);
        return;
    }
    for (size_t i=0; i < curr_root_directory->num_files; i ++){
        directory_file_t dir_file = curr_root_directory->directory_files[i];
        char *file_name = dir_file.file_dir_name;

        if (dir_file.is_directory){
            // printf("dir name: %s\n", dir_file.file_dir_name);
            char *name_w_slash;
            if (dir_file.file_dir_name[strlen(dir_file.file_dir_name)-1] != '/'){
                // printf("bruhhhh\n");

                // printf("   %c ???????", dir_file.file_dir_name[strlen(dir_file.file_dir_name)-1]);
                name_w_slash = add_a_stupid_slash(dir_file.file_dir_name);
            } else {
                name_w_slash = strdup(dir_file.file_dir_name);
            }


            assert(hash_table_contains(dir_map, name_w_slash));

            directory_t * directory = hash_table_get(dir_map, name_w_slash);
            if (!directory->completed){
                directory_traversal(directory, dir_map, index_table, tree_map);
                directory->completed = true;
            } 
            free(name_w_slash);
        } else {
            // it has to be a blob object, make it into the tree entry
            char *full_path = get_full_name(curr_root_directory->name, file_name);
            assert(hash_table_contains(index_table, full_path));
            free(full_path);
        }
    }    
    char *dir_name = get_last_dir(curr_root_directory->name);

    // printf("i can be hashed %s\n", dir_name);
    // hash the stuff and return remember to write name of the tree not the
    // name of the 
    write_tree(curr_root_directory, index_table, tree_map);
    free(dir_name);
    curr_root_directory->completed = true;
}

char *make_tree_from_idx(hash_table_t *tree_map){
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

        current = current->next;
        free(str);
    }

    directory_traversal(root_dir, dir_map, index_file->entries, tree_map);

    char *final_hash = hash_table_get(tree_map, "");


    free_hash_table(dir_map, (free_func_t) free_directory);

    free_index_file(index_file);
    // free_hash_table(dir_tree_map, free_tree);
    return final_hash;

}

commit_t *get_head_commit(bool *detached){
    char *head = read_head_file(detached);

    if (!*detached){
        printf("not detached brodie\n");
    }
    char *hash = malloc(sizeof(char) * (HASH_STRING_LENGTH + 1));
    head_to_hash(head, *detached, hash);

    commit_t *head_commit = read_commit(hash);
    free(hash);
    free(head);
    return head_commit;
}




char* get_unix_timestamp_and_timezone() {
    time_t current_time = time(NULL);
    struct tm* local_time_info = localtime(&current_time);

    // Calculate the timezone offset in hours and minutes
    int timezone_offset_hours = local_time_info->tm_gmtoff / 3600;
    int timezone_offset_minutes = labs((local_time_info->tm_gmtoff % 3600) / 60);

    char* timestamp_and_timezone_str = malloc(30);  // Large enough to hold the timestamp and timezone
    if (timestamp_and_timezone_str == NULL) {
        fprintf(stderr, "Failed to allocate memory for timestamp_and_timezone_str\n");
        exit(1);
    }
    sprintf(timestamp_and_timezone_str, "%ld %+03d%02d", (long)current_time, timezone_offset_hours, timezone_offset_minutes);
    return timestamp_and_timezone_str;
}


char* create_commit_message(const char* tree_hash, const char* commit_message, char **parent_hashes) {
    // Calculate the size of the commit message
    config_t *config = read_global_config();

    config_section_t *config_section = get_section(config, "user");

            
    char * author_email = NULL; //= config_section->properties[0].value;
    char * author_name = NULL; //= config_section->properties[1].value;

    for (size_t i=0; i < config_section->property_count; i++){
        config_property_t property =  config_section->properties[i];
        printf(" %s \n", config_section->properties[i].value);
        if (strcmp(property.key, "email") == 0){
            author_email = config_section->properties[i].value;
            printf("author email: %s\n", author_email);
        }
        if (strcmp(property.key, "name") == 0){
            author_name = config_section->properties[i].value;
            printf("author name: %s\n", author_name);
        }
    }

    if (author_email == NULL || author_name == NULL){
        printf("Author or email is not set in config file");
        exit(1);
    }
    
    
    size_t count = 0;
    size_t parent_len = 0;
    for (size_t i =0 ; parent_hashes[i] != NULL; i++){  
        // printf("parent_hash %s\n", parent_hashes[i]); 
        parent_len += strlen(parent_hashes[i]);
        count++;
    }
    
    char* author_date_unix = get_unix_timestamp_and_timezone();
    char* committer_date_unix = get_unix_timestamp_and_timezone();

    // printf("author date unix %s\n", author_date_unix);

    size_t message_size = strlen("tree \n\nauthor <>  \ncommitter <>  \n\n\n") +
                      strlen(tree_hash) + parent_len + count *strlen("parent ") + 
                      strlen(commit_message) + 
                      2 * (strlen(author_name) + strlen(author_email) + strlen(author_date_unix) + 20);

    char *commit_message_str = malloc(sizeof(char) * message_size);
    if (commit_message_str == NULL) {
        fprintf(stderr, "Failed to allocate memory for commit_message_str\n");
        exit(1);
    }

    strcpy(commit_message_str, "tree ");
    strcat(commit_message_str, tree_hash);
    strcat(commit_message_str, "\n");

    for (size_t i = 0; parent_hashes[i] != NULL; i++) {
        strcat(commit_message_str, "parent ");
        strcat(commit_message_str, parent_hashes[i]);
        strcat(commit_message_str, "\n");
    }

    strcat(commit_message_str, "author ");
    strcat(commit_message_str, author_name);
    strcat(commit_message_str, " <");
    strcat(commit_message_str, author_email);
    strcat(commit_message_str, "> ");
    strcat(commit_message_str, author_date_unix);
    strcat(commit_message_str, "\n");

    strcat(commit_message_str, "committer ");
    strcat(commit_message_str, author_name);
    strcat(commit_message_str, " <");
    strcat(commit_message_str, author_email);
    strcat(commit_message_str, "> ");
    strcat(commit_message_str, committer_date_unix);
    strcat(commit_message_str, "\n\n");

    strcat(commit_message_str, commit_message);
    strcat(commit_message_str, "\n");

    free(author_date_unix);
    free(committer_date_unix);

    // printf("commit message:\n\n\n%s\n", commit_message_str);
    free_config(config);

    return commit_message_str;
}

void commit(const char *commit_message) {
    
    // index_file_t *read_index_file();
    hash_table_t *tree_map = hash_table_init();

    char *tree_hash = make_tree_from_idx(tree_map);
    // printf("tree hash %s\n", tree_hash);
    bool detached;    
    char *head = read_head_file(&detached);


    char *commit_hash = malloc(sizeof(char) * (HASH_STRING_LENGTH + 1));
    bool found = head_to_hash(head, detached, commit_hash);
    if (!found){
        free(commit_hash);
        commit_hash = NULL;
    }

    // make multiple parent hashes
    char **parent_hashes = malloc(sizeof(char *) * 2);
    parent_hashes[0] = commit_hash;
    parent_hashes[1] = NULL;

    char * msg = create_commit_message(tree_hash, commit_message, parent_hashes);
    printf("commit message:\n%s\n", msg);
    object_hash_t hash; 
    write_object(COMMIT, msg, strlen(msg), hash);


    printf("commit hash %s\n", hash);
    free_hash_table(tree_map, free);
    free(msg);
    if (commit_hash != NULL){
        free(commit_hash);
    }
    if (detached){
        write_head_file(hash, detached);
    } else {
        set_branch_ref(head, hash);
    }
    free(head);
    free(parent_hashes);
}
