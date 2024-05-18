#include "push.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config_io.h"
#include "hash_table.h"
#include "linked_list.h"
#include "object_io.h"
#include "ref_io.h"
#include "transport.h"
#include "util.h"

// char *ZERO_HASH = "0000000000000000000000000000000000000000";


void ermm2(char *ref, object_hash_t hash, void *aux){
    // printf("ref received: %s\n", ref);
    // printf("hash: %s\n", hash);

    char * a = strdup(ref);

    char **refs = split_path_into_directories(a);
    if (strcmp(refs[1], "heads") == 0){
        hash_table_t *table = (hash_table_t *)aux;
        hash_table_add(table, ref, strdup((char *)hash));    
    }
    free(a);
    free(refs);
}

void get_commit_hashes_to_push(hash_table_t* hash_table, char *hash, char *remote_hash){
    printf("hash %s\n", hash);
    commit_t *commit = read_commit(hash);
    if (remote_hash != NULL && strcmp(hash, remote_hash) == 0) {
        free_commit(commit);
        return;
    }
    if (hash_table_contains(hash_table, hash)){
        free_commit(commit);
        return;
    }
    hash_table_add(hash_table, hash, "aaaaaaaaaaaa");


    // needs to work with merge thingies
    printf("commit %s\n", hash);
    if (commit->parents > 0){
        // Check if the current hash is the remote hash
        for (size_t i=0; i < commit->parents; i++){
            char * parent_hash = commit->parent_hashes[i];
            get_commit_hashes_to_push(hash_table, parent_hash, remote_hash);
        }
    }
    
    free_commit(commit);

}

char *make_head_ref_from_branch(char *branch){
    char * ref = malloc(sizeof(char) * (strlen("refs/heads/") + strlen(branch) + 1));
    strcpy(ref, "refs/heads/");
    strcat(ref, branch);
    return ref;
}

void add_hash_list_tree(char *tree_hash, transport_t *transport,  hash_table_t* hash_list){
    tree_t *tree = read_tree(tree_hash);
    for (size_t i=0; i < tree->entry_count; i++){
        tree_entry_t tree_entry = tree->entries[i];
        if (strcmp("104e6a4a7a0872bd86e1ecfd8a44835f221a638a", tree_entry.hash) == 0){
            printf("WE GOT EM : %s\n", tree_entry.name);
        }        
        if (tree_entry.mode == MODE_DIRECTORY || tree_entry.mode == MODE_EXECUTABLE || tree_entry.mode == MODE_FILE ||tree_entry.mode == MODE_SYMLINK){
            hash_table_add(hash_list, tree_entry.hash, "yooo");
        }
        if (tree_entry.mode == MODE_DIRECTORY){
            add_hash_list_tree(tree_entry.hash, transport, hash_list);
        }
    }
    free_tree(tree);
}

void add_hashes_and_content(char *commit_hash, transport_t *transport, hash_table_t* hash_set){

    hash_table_add(hash_set, commit_hash, "yoooo");

    commit_t *commit = read_commit(commit_hash);

    char * tree_hash = commit->tree_hash;
    hash_table_add(hash_set, tree_hash, "yoooo");

    add_hash_list_tree(tree_hash, transport, hash_set);
    free_commit(commit);
}

char *get_branch_config(char *branch){
    char *branch_config = malloc(sizeof(char) * strlen("branch \"") + strlen(branch) + 2);
    strcpy(branch_config, "branch \"");
    strcat(branch_config, branch);
    strcat(branch_config, "\"");
    return branch_config;
}

char *get_remote(config_t *config, char * branch){

    config_section_t * branch_sec = get_branch_section(config, branch);
    if (branch_sec == NULL){
        printf("branch %s doesn't have a config section\n", branch);
        exit(1);
    }
    // TODO: "If a branch is new, it may not have a config section yet;"

    char *remote = get_property_value(branch_sec, "remote");
    if (remote == NULL){
        printf("failed to push to branch %s, does not exist in config\n", branch);
        exit(1);
    }
    return remote;
}

// get the remotes and the branches that correspond to that remote
hash_table_t *get_remotes(size_t branch_count, const char**branch_names, config_t * config){
    hash_table_t *remote_table = hash_table_init();
    for (size_t i = 0; i < branch_count; i++){
        char *branch_name = branch_names[i];
        char *remote = get_remote(config, branch_name);
        linked_list_t *pushes = hash_table_get(remote_table, remote);
        if (pushes == NULL){
            linked_list_t* ll = init_linked_list();
            list_push_back(ll, strdup(branch_name));
            hash_table_add(remote_table, remote, ll);
        } else {
            list_push_back(pushes, strdup(branch_name));
        }
    }
    return remote_table;
}

void push_pack(hash_table_t *hash_set, transport_t *transport){
    // iterate over the hash_set push the objects

    list_node_t *hash_node = key_set(hash_set);

    while (hash_node != NULL){
        char *hash = hash_node->value;
        object_type_t type;
        size_t length;
        uint8_t *contents = read_object(hash, &type, &length);

        // printf("contents %s\n", contents);
        if (contents != NULL){
            send_pack_object(transport, type, contents, length);
            free(contents);
        }

        
        hash_node = hash_node->next;
    }
}

// Returns the hash table of objects that we need to push
hash_table_t *push_branches_for_remote(linked_list_t *branch_list, char *remote, hash_table_t * ref_to_hash, transport_t *transport){
    list_node_t *branch = branch_list->head;
    hash_table_t *hash_set = hash_table_init();

    while (branch != NULL){
        char * branch_name = branch->value;
        object_hash_t my_remote_hash;
        memcpy(my_remote_hash, ZERO_HASH, sizeof(my_remote_hash));
        bool found = get_remote_ref(remote, branch_name, my_remote_hash);
        if (!found){
            printf("branch %s remote was not found\n", branch_name);
            // exit(1);
            set_remote_ref(remote, branch_name, ZERO_HASH);
        }

        printf("branch: %s\n", branch_name);
        char *ref = make_head_ref_from_branch(branch_name);
        printf("whaaaaa %s\n", ref);
        char *remote_hash = hash_table_get(ref_to_hash, ref);
        printf("remote_hash: %s\n", remote_hash);
        printf("my hash: %s\n", my_remote_hash);
        if (strcmp(my_remote_hash, ZERO_HASH) == 0 || remote_hash == NULL){
            printf("lets gooooo\n");
        } else {
            if ((strcmp(remote_hash, my_remote_hash) != 0) ){
                printf("you gotta fetch first\n");
                exit(1);
            }
        }


        object_hash_t curr_hash;
        bool found_branch = get_branch_ref(branch_name, curr_hash);
        if (!found_branch){
            printf("Branch: %s was not found\n", branch_name);
            exit(1);
        }

        if (remote_hash != NULL && strcmp(curr_hash, remote_hash) == 0){
            printf("Already up to date\n");       
        } else {
            char *curr_hash_copy = strdup(curr_hash);            
            hash_table_t *hashes_to_push = hash_table_init();
            get_commit_hashes_to_push(hashes_to_push, curr_hash, remote_hash);

            printf("curr hash update %s\n", curr_hash_copy);

            send_update(transport, ref, remote_hash, curr_hash_copy);
            free(curr_hash_copy);

            // send one update for each branch
            
            list_node_t *hash_to_push = key_set(hashes_to_push);
            while (hash_to_push != NULL){
                char * commit_hash = hash_to_push->value;
                add_hashes_and_content(commit_hash, transport, hash_set);

                hash_to_push = hash_to_push->next;
            }
            free_hash_table(hashes_to_push, NULL);
            // TODO: Not efficient think of a better way????
            // for (size_t i=0; hashes_to_push[i] != NULL; i++){
            // }
        }
        free(ref);
        branch = branch->next;
    }
    finish_updates(transport);
    size_t brodie = hash_table_size(hash_set);
    if (brodie == 0){
        free_hash_table(hash_set, NULL);
        return NULL;
    }


    return hash_set;
}

void receive_updated_refs(char *ref, void *aux){
    printf("ref: '%s' successfully received\n", ref);
    linked_list_t *ll = (linked_list_t *) aux;
    list_push_back(ll, strdup(ref));
}

// maps the branches to it's hashes
// hash_table_t *get_remote_hash_refs(char *curr_remote, linked_list_t *branches){
//     list_node_t *curr_branch = branches->head;

//     hash_table_t *remote_hash_refs = hash_table_init();
//     while (curr_branch != NULL){
//         char *branch_name = curr_branch->value;
        
//         object_hash_t hash;
//         bool found_branch = get_branch_ref(branch_name, hash);
//         if (!found_branch){
//             printf("COuld not find branch\n");
//         }
//         hash_table_add(remote_hash_refs, branch_name, strdup(hash));
//         curr_branch = curr_branch->next;
//     }
//     return remote_hash_refs;
// }



void set_remote_branch_success(linked_list_t *successful_branches, char *remote){
    list_node_t * good_refs = successful_branches->head;
    while (good_refs != NULL){
        char * ref = good_refs->value;
        printf("ref success: %s\n", ref);
        char * branch = get_last_dir(ref);
        printf("branch: %s\n", branch);
        object_hash_t curr_hash;
        bool found_branch = get_branch_ref(branch, curr_hash);
        if (!found_branch){
            printf("Branch: %s was not found\n", branch);
            exit(1);
        }

        printf("curr hash: %s\n", curr_hash);
        set_remote_ref(remote, branch, curr_hash);
        good_refs = good_refs->next;
        free(branch);
    }
}

char **get_all_branches(size_t *branch_count, linked_list_t* branches) {
    // Allocate memory for the branch_names array
    char **branch_names = malloc((*branch_count) * sizeof(char *));
    if (branch_names == NULL) {
        fprintf(stderr, "Failed to allocate memory.\n");
        return NULL;
    }

    // Reset branch pointer to the head of the list
    list_node_t *branch = branches->head;

    // Fill the branch_names array
    int i = 0;
    while (branch != NULL){
        branch_names[i] = strdup(branch->value);
        branch = branch->next;
        i++;
    }

    free_linked_list(branches, free);

    return branch_names;
}

config_t *copy_config_and_add_section(const config_t *old_config, const char *branch_name, const char *remote_value) {
    // Allocate memory for the new config
    config_t *new_config = malloc(sizeof(config_t) + (old_config->section_count + 1) * sizeof(config_section_t));
    if (new_config == NULL) {
        fprintf(stderr, "Failed to allocate memory.\n");
        return NULL;
    }

    // Copy the sections from the old config to the new one
    memcpy(new_config->sections, old_config->sections, old_config->section_count * sizeof(config_section_t));

    // Increase the section count
    new_config->section_count = old_config->section_count + 1;

    // Initialize the new section
    config_section_t *new_section = &new_config->sections[new_config->section_count - 1];

    char section_name[1000];
    snprintf(section_name, sizeof(section_name), "branch \"%s\"", branch_name);

    new_section->name = strdup(section_name);
    new_section->property_count = 2;
    new_section->properties = malloc(new_section->property_count * sizeof(config_property_t));

    // Initialize the "remote" property
    new_section->properties[0].key = strdup("remote");
    new_section->properties[0].value = strdup(remote_value);

    // Initialize the "merge" property
    
    new_section->properties[1].key = strdup("merge");

    // Create the "merge" value
    char merge[1000];
    snprintf(merge, sizeof(merge), "refs/heads/%s", branch_name);
    new_section->properties[1].value = strdup(merge);

    return new_config;
}


void push(size_t branch_count, const char **branch_names, const char *set_remote) {
    free("bruh");
    if (set_remote != NULL){
        printf("yuh\n");
        config_t *config = read_config();
        if (get_remote_section(config, set_remote) == NULL){
            printf("error: src refspec %s does not match any\n", set_remote);
            return;
        }
        for (size_t i=0; i < branch_count; i++){
            char *branch_name = branch_names[i];
            // Create a new config with the added section
            printf("branch name: %s\n", branch_name);

            config_section_t *config_sec = get_branch_section(config, branch_name);
            if (config_sec != NULL){
                printf("????\n");
                set_property_value(config_sec, "remote", set_remote);
                write_config(config);
                // while (true){
                //     printf("yooooo what is up guys \n");
                // }
                continue;
            } else{
                config_t *new_config = copy_config_and_add_section(config, branch_name, set_remote);
                write_config(new_config);

                set_remote_ref(set_remote, branch_name, ZERO_HASH);

                // Free the old config and set the new one as the current config
                free_config(config);
                config = new_config;
            }
        }
        // free_config(config);
    }

    if (branch_count == 0 && branch_names == NULL){
        linked_list_t * branches = list_branch_refs();
        list_node_t *branch = branches->head;

        while (branch != NULL){
            branch = branch->next;
            (branch_count)++;
        }
        branch_names = get_all_branches(&branch_count, branches);
    }

    config_t *config = read_config();
    
    // hash table from remotes to its corresponding branches
    hash_table_t * remote_table = get_remotes(branch_count, branch_names, config);
    list_node_t *curr_remote = key_set(remote_table);

    while (curr_remote != NULL){
        char * remote = curr_remote->value;

        config_section_t *remote_sec = get_remote_section(config, remote);
        if (remote_sec == NULL){
            printf("config couldn't find thee remote\n");
            exit(1);
        }
        char *url = get_url(remote_sec);
        printf("url: %s\n", url);
        transport_t * transport = open_transport(PUSH, url);
        hash_table_t *ref_to_hash = hash_table_init(); 
        receive_refs(transport, ermm2, ref_to_hash);

        linked_list_t *branch_list = hash_table_get(remote_table, remote);
        assert(branch_list != NULL);
        
        hash_table_t *hash_set = push_branches_for_remote(branch_list, remote, ref_to_hash, transport);

        if (hash_set == NULL){
            close_transport(transport);
        } else {
            // CLEAN UP THIS 
            size_t num_objects = hash_table_size(hash_set);

            start_pack(transport, num_objects);
            push_pack(hash_set, transport);

            finish_pack(transport);

            linked_list_t *successful_refs = init_linked_list();

            check_updates(transport, receive_updated_refs, successful_refs);

            set_remote_branch_success(successful_refs, remote);

            close_transport(transport);

            free_linked_list(successful_refs, free);
            free_hash_table(hash_set, NULL);
        }

        free_hash_table(ref_to_hash, free);
        // NEED TO UPDATE THE CURRENT REF AND MERGE WHATEVER
        curr_remote = curr_remote->next;
        free(url);
    }

    free_config(config);
    free_hash_table(remote_table, (free_func_t) free_linked_list_char);
}
    

