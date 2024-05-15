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

    printf("Not implemented.\n");

    for (size_t i=0; i < branch_count; i ++){
        char *branch = branch_names[i];        
    
        config_t *config = read_config();
        for (size_t i=0; i < config->section_count; i++){
            config_section_t sec = config->sections[i];
            printf("sec name: %s\n", sec.name);
            for (size_t i = 0; i < sec.property_count; i++){
                printf("property key : %s\n", sec.properties[i].key);
                printf("properties value: %s\n", sec.properties[i].value);

            }

        }
        free_config(config);
        // config_section_t *remote = get_remote_section(config, set_remote);
    
        // char *url = get_url(remote);
        // printf("url: %s\n", url);

        // transport_t * transport = open_transport(FETCH, url);
        // hash_table_t *ref_to_hash = hash_table_init(); 
        // receive_refs(transport, ermm2, ref_to_hash);
        // close_transport(transport);
        // free(url);
    }
    
    // list_node_t *ref_node = key_set(ref_to_hash);


    // finish_wants(transport);

    
}
