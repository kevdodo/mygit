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
}

void fetch_remote(const char *remote_name, config_section_t *remote) {
    char *url = get_url(remote);
    printf("url: %s\n", url);

    transport_t * transport = open_transport(FETCH, url);
    receive_refs(transport, ermm, NULL);

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
