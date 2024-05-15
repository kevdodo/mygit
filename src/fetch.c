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

struct list_node {
   list_node_t *next;
   void *value;
};
struct linked_list {
    list_node_t *head;
    list_node_t **tail;
};

char *get_url(config_section_t *remote) {
    for (size_t i = 0; i < remote->property_count; i++) {
        config_property_t property = remote->properties[i];
        if (strcmp(property.key, "url") == 0) {
            char *url = malloc(strlen(property.value) + 1);
            if (url == NULL) {
                fprintf(stderr, "Failed to allocate memory for URL.\n");
                return NULL;
            }
            strcpy(url, property.value);
            return url;
        }
    }
    return NULL;
}

void ermm(char *ref, object_hash_t hash, void *aux){
    printf("ref: %s\n", ref);
    printf("hash: %s\n", hash);

    hash_table_t *table = (hash_table_t *)aux;
    hash_table_add(table, ref, hash);
}

void fetch_remote(const char *remote_name, config_section_t *remote) {
    char *url = get_url(remote);
    printf("url: %s\n", url);

    transport_t * transport = open_transport(FETCH, url);
    hash_table_t *ref_to_hash = hash_table_init(); 
    receive_refs(transport, ermm, ref_to_hash);
    
    list_node_t *ref_node = key_set(ref_to_hash);
    printf("remote name: %s\n", remote_name);

    // // iterating through the commit file to track delted files


    while (ref_node != NULL){
        char *ref = ref_node->value;
        printf("ref name: %s\n", ref);

        object_hash_t hash;
        bool remote_ref = get_remote_ref(remote_name, ref, hash);
        if (!remote_ref){
            printf("Remote Ref : '%s' not found\n", remote_name);
            exit(1);
        }

        // char * hash = hash_table_get(ref_to_hash, ref); 
        send_want(transport, hash);
        finish_wants(transport);

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
