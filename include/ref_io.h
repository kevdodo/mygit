#ifndef REF_IO_H
#define REF_IO_H

#include <stdbool.h>
#include "constants.h"
#include "linked_list.h"

char *read_head_file(bool *detached);
void write_head_file(const char *contents, bool detached);
bool head_to_hash(const char *head, bool detached, object_hash_t hash);

bool get_branch_ref(const char *branch, object_hash_t hash);
void set_branch_ref(const char *branch, const object_hash_t hash);

bool get_remote_ref(const char *remote, const char *ref, object_hash_t hash);
void set_remote_ref(const char *remote, const char *ref, const object_hash_t hash);

linked_list_t *list_branch_refs(void);

#endif // #ifndef REF_IO_H
