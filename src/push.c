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
    printf("ref: %s\n", ref);
    printf("hash: %s\n", hash);

    hash_table_t *table = (hash_table_t *)aux;
    hash_table_add(table, ref, strdup((char *)hash));
    // free(ref);
}

char **get_commit_hashes_to_push(char *hash, char *remote_hash){
    // printf("hash: %s", hash);
    commit_t *commit = read_commit(hash);
    char **hashes = NULL;
    size_t count = 0;

    while (commit != NULL){
        object_hash_t *parent_hashes = commit->parent_hashes;
        printf("commit %s\n", hash);
        if (commit->parents > 0){
            // Check if the current hash is the remote hash
            if (strcmp(hash, remote_hash) == 0) {
                break;
            }

            // Add the current hash to the array
            hashes = realloc(hashes, sizeof(char *) * (count + 1));
            hashes[count] = strdup(hash);
            count++;

            memcpy(hash, &(commit->parent_hashes[0]), sizeof(object_hash_t));
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

char * make_head_ref(char *branch){
    char * ref = malloc(sizeof(char) * (strlen("refs/heads/") + strlen(branch) + 1));
    strcpy(ref, "refs/heads/");
    strcat(ref, branch);
    return ref;
}

void send_updates_for_commit(char *commit_hash){

}
            // char *blob = read_object(new_hash, &obj_type, &length);
            // if (blob == NULL) {
            //     printf("Failed to open object: %s\n", new_hash);
            //     exit(1);
            // }

void push(size_t branch_count, const char **branch_names, const char *set_remote) {
    for (size_t i=0; i < branch_count; i ++){
        char *branch = branch_names[i];  
        char *branch_config = malloc(sizeof(char) * strlen("branch \"") + strlen(branch) + 2);
        strcpy(branch_config, "branch \"");

        strcat(branch_config, branch);
        strcat(branch_config, "\"");

        printf("branch config noooo way: %s\n", branch_config);
        config_t *config = read_config();


        char *remote;
        // todo: what to do with merge???
        char *merge;
        for (size_t i=0; i < config->section_count; i++){
            config_section_t sec = config->sections[i];
            if (strcmp(branch_config, sec.name) == 0){
                size_t name_len = strlen(sec.name);
                for (size_t i = 0; i < sec.property_count; i++){
                    if (strcmp(sec.properties[i].key, "remote") == 0 ){
                        remote = sec.properties[i].value;
                    }
                    if (strcmp(sec.properties[i].key, "merge") == 0 ){
                        merge = sec.properties[i].value;
                    }
                }
            }
        }
        
        // TODO: "If a branch is new, it may not have a config section yet;"
        if (remote == NULL){
            printf("failed to push to branch %s, does not exist in config\n", branch);
            exit(1);
        }
        printf("we need to push to  : %s\n", remote);

        config_section_t *remote_sec = get_remote_section(config, remote);
        char *url = get_url(remote_sec);
        printf("url: %s\n", url);

        transport_t * transport = open_transport(PUSH, url);
        hash_table_t *ref_to_hash = hash_table_init(); 
        receive_refs(transport, ermm2, ref_to_hash);

        // list_node_t *ls_node = key_set(ref_to_hash);

        object_hash_t my_remote_hash;
        bool found = get_remote_ref(remote, branch, my_remote_hash);
        if (!found){
            printf("branch %s was not found\n", branch);
            exit(1);
        }
        // char * branch_name = get_last_dir(ref);

        char *remote_hash = hash_table_get(ref_to_hash, branch);
        printf("remote_hash: %s\n", remote_hash);
        printf("my hash: %s\n", my_remote_hash);

        if (remote_hash == NULL || strcmp(remote_hash, my_remote_hash) != 0){
            printf("you gotta fetch first\n");
            exit(1);
        }
        object_hash_t curr_hash;
        bool found_branch = get_branch_ref(branch, curr_hash);
        if (!found_branch){
            printf("branch %s was not found", branch);
            exit(1);
        }

        char **hashes_to_push = get_commit_hashes_to_push(curr_hash, remote_hash);
        char * old_hash = remote_hash;
        for (size_t i=0; hashes_to_push[i] != NULL; i++){
            printf("hash to push: %s\n", hashes_to_push[i]);
            // send_update(transport, ref, old_hash, hashes_to_push[i])
            old_hash = hashes_to_push[i];
        }

        // send_update(transport, branch, remote_hash, curr_hash);
        finish_updates(transport);

        // we can push the branch!!



        // while (ls_node != NULL){
        //     char *ref = ls_node->value;
        //     printf("ref here: %s\n", ref);
        //     // TODO: what to do with the head ref prbably update it huh
        //     if (strcmp("HEAD", ref) == 0){
        //         ls_node = ls_node->next;
        //         continue;
        //     }

        //     char * branch_name = get_last_dir(ref);
        //     printf("%s\n", bruh);

        //     object_hash_t myhash;
        //     bool got_ref = get_remote_ref(remote, bruh, myhash);
        //     if (!got_ref){
        //         printf("what\n");
        //         exit(1);
        //     }
            
        //     printf("myhash: %s\n", myhash);

        //     // compare myhash with the remote hash to see if we need to do a git fetch/pull
        //     // if (strcmp(myhash, ))

        //     ls_node = ls_node->next;
        // }

        // send_update(transport, NULL, )
        close_transport(transport);
        // free(url);


        
        free_config(config);
        free(branch_config);
        free(url);
    }
    
    // list_node_t *ref_node = key_set(ref_to_hash);


    // finish_wants(transport);

    
}
