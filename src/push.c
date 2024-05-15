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
    hash_table_add(table, ref, hash);
}


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
            printf("sec name: %s\n", sec.name);
            if (strcmp(branch_config, sec.name) == 0){
                size_t name_len = strlen(sec.name);
                for (size_t i = 0; i < sec.property_count; i++){
                    printf("property key : %s\n", sec.properties[i].key);
                    printf("properties value: %s\n", sec.properties[i].value);
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
        printf("remote!!!! : %s\n", remote);

        config_section_t *remote_sec = get_remote_section(config, remote);
        char *url = get_url(remote_sec);
        printf("url: %s\n", url);

        transport_t * transport = open_transport(FETCH, url);
        hash_table_t *ref_to_hash = hash_table_init(); 
        receive_refs(transport, ermm2, ref_to_hash);

        list_node_t *ls_node = key_set(ref_to_hash);

        while (ls_node != NULL){
            char *ref = ls_node->value;
            printf("ref: %s", ref);
            ls_node = ls_node->next;
        }

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
