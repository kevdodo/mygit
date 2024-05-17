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


void ermm2(char *ref, object_hash_t hash, void *aux){
    printf("ref received: %s\n", ref);
    printf("hash: %s\n", hash);

    hash_table_t *table = (hash_table_t *)aux;
    hash_table_add(table, ref, strdup((char *)hash));
    // free(ref);
}

char **get_commit_hashes_to_push(char *hash, char *remote_hash){
    // printf("hash: %s", hash);

    commit_t *commit = read_commit(hash);
    char **hashes = malloc(sizeof(char *));
    hashes[0] = strdup(hash);
    size_t count = 1;

    while (commit != NULL){
        object_hash_t *parent_hashes = commit->parent_hashes;
        printf("commit %s\n", hash);
        if (commit->parents > 0){
            // Check if the current hash is the remote hash
            if (remote_hash != NULL && strcmp(hash, remote_hash) == 0) {
                break;
            }

            // Add the current hash to the array
            hashes = realloc(hashes, sizeof(char *) * (count + 1));
            hashes[count] = strdup(hash);
            count++;

            memcpy(hash, &(parent_hashes[0]), sizeof(object_hash_t));

            free_commit(commit);

            commit = read_commit(hash);
        } else {
            free_commit(commit);
            commit = NULL;
        }
    }

    // Add a NULL pointer at the end of the array
    hashes = realloc(hashes, sizeof(char *) * (count + 1));
    hashes[count] = NULL;

    return hashes;
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
        hash_table_add(hash_list, strdup(tree_entry.hash), "yooo");
        if (tree_entry.mode == MODE_DIRECTORY){
            add_hash_list_tree(tree_entry.hash, transport, hash_list);
        }
    }
    free_tree(tree);
}

void add_hashes_and_content(char *commit_hash, transport_t *transport, hash_table_t* hash_set){

    hash_table_add(hash_set, strdup(commit_hash), "yoooo");

    commit_t *commit = read_commit(commit_hash);

    char * tree_hash = strdup(commit->tree_hash);
    hash_table_add(hash_set, strdup(tree_hash), "yoooo");

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
    char *merge = get_property_value(branch_sec, "merge");
    if (remote == NULL){
        printf("failed to push to branch %s, does not exist in config\n", branch);
        exit(1);
    }
    printf("we need to push to  : %s\n", remote);
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

        send_pack_object(transport, type, contents, length);
        
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
        bool found = get_remote_ref(remote, branch_name, my_remote_hash);
        if (!found){
            printf("branch %s was not found\n", branch_name);
            exit(1);
        }

        printf("branch: %s\n", branch_name);
        char *ref = make_head_ref_from_branch(branch_name);

        char *remote_hash = hash_table_get(ref_to_hash, ref);
        printf("remote_hash: %s\n", remote_hash);
        printf("my hash: %s\n", my_remote_hash);

        if (remote_hash == NULL || strcmp(remote_hash, my_remote_hash) != 0){
            printf("you gotta fetch first\n");
            exit(1);
        }

        object_hash_t curr_hash;
        bool found_branch = get_branch_ref(branch_name, curr_hash);
        if (!found_branch){
            printf("Branch: %s was not found\n", branch_name);
            exit(1);
        }

        char **hashes_to_push = get_commit_hashes_to_push(curr_hash, remote_hash);

        // TODO: Handle deleting the ref
        char *old_hash = remote_hash;
        for (size_t i=0; hashes_to_push[i] != NULL; i++){
            printf("hash to push: %s\n", hashes_to_push[i]);
            send_update(transport, ref, old_hash, hashes_to_push[i]);
            old_hash = hashes_to_push[i];
        }

        finish_updates(transport);

        // TODO: Not efficient think of a better way????
        for (size_t i=0; hashes_to_push[i] != NULL; i++){
            add_hashes_and_content(hashes_to_push[i], transport, hash_set);
        }
        branch = branch->next;
    }
    return hash_set;
}

void receive_updated_refs(char *ref, void *aux){
    printf("ref: '%s' successfully received\n", ref);
    linked_list_t *ll = (linked_list_t *) aux;
    list_push_back(ll, strdup(ref));
}


hash_table_t *get_remote_hash_refs(char *curr_remote, linked_list_t *branches){
    list_node_t *curr_branch = branches->head;

    hash_table_t *remote_hash_refs = hash_table_init();
    while (curr_branch != NULL){
        char *branch_name = curr_branch->value;
        
        object_hash_t hash;
        bool found_branch = get_branch_ref(branch_name, hash);
        if (!found_branch){
            printf("COuld not find branch\n");
        }
        hash_table_add(remote_hash_refs, strdup(branch_name), strdup(hash));
        curr_branch = curr_branch->next;
    }
    return remote_hash_refs;
}

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
        set_remote_ref(remote, ref, curr_hash);
        good_refs = good_refs->next;
        free(branch);
    }
}


void push(size_t branch_count, const char **branch_names, const char *set_remote) {

    if (branch_count == 0 && branch_names == NULL){
        printf("Not implemented \n");
        return;
    }
    config_t *config = read_config();
    // hash table from remotes to its corresponding branches
    hash_table_t * remote_table = get_remotes(branch_count, branch_names, config);
    list_node_t *curr_remote = key_set(remote_table);

    while (curr_remote != NULL){
        char * remote = curr_remote->value;

        config_section_t *remote_sec = get_remote_section(config, remote);
        char *url = get_url(remote_sec);
        printf("url: %s\n", url);
        transport_t * transport = open_transport(PUSH, url);
        hash_table_t *ref_to_hash = hash_table_init(); 
        receive_refs(transport, ermm2, ref_to_hash);

        linked_list_t *branch_list = hash_table_get(remote_table, remote);
        assert(branch_list != NULL);
        
        hash_table_t *hash_set = push_branches_for_remote(branch_list, remote, ref_to_hash, transport);
        
        size_t num_objects = hash_table_size(hash_set);

        start_pack(transport, num_objects);
        push_pack(hash_set, transport);
        finish_pack(transport);

        hash_table_t * table = get_remote_hash_refs(remote, branch_list);

        linked_list_t *successful_refs = init_linked_list();

        check_updates(transport, receive_updated_refs, successful_refs);

        set_remote_branch_success(successful_refs, remote);

        close_transport(transport);

        free_linked_list(successful_refs, free);


        // NEED TO UPDATE THE CURRENT REF AND MERGE WHATEVER


        curr_remote = curr_remote->next;
        free(url);
    }

    free_config(config);
    free_hash_table(remote_table, (free_func_t) free_linked_list_char);
}
    

