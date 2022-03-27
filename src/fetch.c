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

void fetch_remote(const char *remote_name, config_section_t *remote) {
    (void)remote_name;
    (void)remote;
    printf("Not implemented.\n");
    exit(1);
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
            fetch_remote(remote_name, section);
            free(remote_name);
        }
    }
    free_config(config);
}
