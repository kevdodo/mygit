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


void push(size_t branch_count, const char **branch_names, const char *set_remote) {
    (void)branch_count;
    (void)branch_names;
    (void)set_remote;
    printf("Not implemented.\n");
    exit(1);
}
