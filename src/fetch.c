#include "fetch.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "config_io.h"
#include "hash_table.h"
#include "object_io.h"
#include "ref_io.h"
#include "transport.h"
#include "util.h"



void ermm(char *ref, object_hash_t hash, void *aux){
    printf("ref: %s\n", ref);
    printf("hash: %s\n", hash);

    hash_table_t *table = (hash_table_t *)aux;
    hash_table_add(table, ref, hash);
}

char ** go_through_local_commits(object_hash_t hash, char * hash_fetched){
    char ** hashes_to_fetch = NULL;
    size_t hashes_cnt = 0;
    printf("hash fetched: %s\n", hash);
    commit_t *commit = read_commit(hash);
    while (commit != NULL){
        object_hash_t *parent_hashes = commit->parent_hashes;
        for (size_t i = 0; i < commit->parents; i++) {
            // hash was found in commit history
            hashes_to_fetch = realloc(hashes_to_fetch, hashes_cnt + 1);
            if (strcmp(parent_hashes[i], hash_fetched) == 0){
                return false;
            }
            hashes_to_fetch[hashes_cnt] = strdup(parent_hashes[i]);
            hashes_cnt++;
        }    

        if (commit->parents > 0){
            memcpy(hash, &(commit->parent_hashes[0]), sizeof(object_hash_t));
            free_commit(commit);
            commit = read_commit(hash);
        } else {
            free_commit(commit);
            commit = NULL;
        }
    }
    return hashes_to_fetch;
}

void fetch_remote(const char *remote_name, config_section_t *remote) {
    char *url = get_url(remote);
    printf("url: %s\n", url);

    transport_t * transport = open_transport(FETCH, url);
    hash_table_t *ref_to_hash = hash_table_init(); 
    receive_refs(transport, ermm, ref_to_hash);
    
    list_node_t *ref_node = key_set(ref_to_hash);
    printf("remote name: %s\n", remote_name);


    while (ref_node != NULL){
        char *ref = ref_node->value;
        printf("ref name: %s\n", ref);

        object_hash_t hash; 

        bool remote_ref = get_remote_ref(remote_name, ref, hash);
        if (!remote_ref){
            printf("Remote Ref : '%s' not found\n", remote_name);
            exit(1);
        }

        // char **local_commits = go_through_local_commits(hash, hash_fetched);

        // for (size_t i=0; local_commits[i] != NULL; i++){
        //     printf("local commit hash: %s", local_commits[i]);
        // }

        ref_node = ref_node->next;
    }


    finish_wants(transport);

    close_transport(transport);
    
    free(url);
    // exit(1);
}

void fetch(const char *remote_name) {
    config_t *config = read_config();
    config_section_t *remote = get_remote_section(config, remote_name);
    if (remote == NULL) {
        fprintf(stderr, "No such remote: %s\n", remote_name);
        exit(1);
    }

    fetch_remote(remote_name, remote);
    free_config(config);
}

void fetch_all(void) {
    config_t *config = read_config();
    for (size_t i = 0; i < config->section_count; i++) {
        config_section_t *section = &config->sections[i];
        if (is_remote_section(section)) {
            char *remote_name = get_remote_name(section);
            // printf("remote name: %s", remote_name);
            fetch_remote(remote_name, section);
            free(remote_name);
        }
    }
    free_config(config);
}
