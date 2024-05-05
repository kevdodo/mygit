#include "ref_io.h"
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

// HEAD file pointing to a ref (e.g. master) starts with this prefix
const char REF_PREFIX[] = "ref: refs/heads/";
// The HEAD file
const char HEAD_FILE[] = ".git/HEAD";
// The branches directory (contains a file for each branch name)
const char BRANCH_REFS_DIR[] = ".git/refs/heads/";
// The remote branches directory (contains a directory for each remote)
const char REMOTE_REFS_DIR[] = ".git/refs/remotes/";

char *read_head_file(bool *detached) {
    FILE *file = fopen(HEAD_FILE, "r");
    if (file == NULL) {
        fprintf(stderr, "fatal: HEAD file not found\n");
        assert(false);
    }

    // Have to strip newline at end
    size_t size = get_file_size(file) / sizeof(char) - 1;
    char *contents = malloc(sizeof(char[size + 1]));
    assert(contents != NULL);
    size_t read = fread(contents, sizeof(char), size, file);
    assert(read == size);
    fclose(file);
    contents[size] = '\0';

    *detached = !starts_with(contents, REF_PREFIX);
    if (*detached) assert(size == HASH_STRING_LENGTH);
    else {
        size_t prefix_length = strlen(REF_PREFIX);
        size = sizeof(char[size - prefix_length + 1]);
        memmove(contents, contents + prefix_length, size);
        contents = realloc(contents, size);
        assert(contents != NULL);
    }
    return contents;
}

void write_head_file(const char *contents, bool detached) {
    FILE *file = fopen(HEAD_FILE, "w");
    if (file == NULL) {
        fprintf(stderr, "fatal: could not write to HEAD\n");
        assert(false);
    }

    int result;
    if (detached) assert(strlen(contents) == HASH_STRING_LENGTH);
    else {
        result = fputs(REF_PREFIX, file);
        assert(result != EOF);
    }
    result = fputs(contents, file);
    assert(result != EOF);
    result = fputc('\n', file);
    assert(result != EOF);
    fclose(file);
}

bool head_to_hash(const char *head, bool detached, object_hash_t hash) {
    if (detached) {
        memcpy(hash, head, sizeof(object_hash_t));
        return true;
    }
    
    return get_branch_ref(head, hash);
}

FILE *open_branch_ref(const char *branch, char *mode) {
    char filename[strlen(BRANCH_REFS_DIR) + strlen(branch) + 1];
    strcpy(filename, BRANCH_REFS_DIR);
    strcat(filename, branch);
    if (mode[0] == 'w') make_parent_dirs(filename);
    return fopen(filename, mode);
}

bool read_hash(FILE *file, object_hash_t hash) {
    if (file == NULL) return false;

    size_t read = fread(hash, sizeof(char), HASH_STRING_LENGTH, file);
    assert(read == HASH_STRING_LENGTH);
    hash[HASH_STRING_LENGTH] = '\0';
    fclose(file);
    return true;
}
void write_hash(FILE *file, const object_hash_t hash) {
    size_t written = fwrite(hash, sizeof(char), HASH_STRING_LENGTH, file);
    assert(written == HASH_STRING_LENGTH);
    int result = fputc('\n', file);
    assert(result != EOF);
    fclose(file);
}

bool get_branch_ref(const char *branch, object_hash_t hash) {
    return read_hash(open_branch_ref(branch, "r"), hash);
}
void set_branch_ref(const char *branch, const object_hash_t hash) {
    FILE *file = open_branch_ref(branch, "w");
    if (file == NULL) {
        fprintf(stderr, "Failed to write branch: %s\n", branch);
        assert(false);
    }
    write_hash(file, hash);
}

FILE *open_remote_ref(const char *remote, const char *ref, char *mode) {
    char filename[strlen(REMOTE_REFS_DIR) + strlen(remote) + 1 + strlen(ref) + 1];
    strcpy(filename, REMOTE_REFS_DIR);
    strcat(filename, remote);
    strcat(filename, "/");
    strcat(filename, ref);
    if (mode[0] == 'w') make_parent_dirs(filename);
    return fopen(filename, mode);
}

bool get_remote_ref(const char *remote, const char *ref, object_hash_t hash) {
    return read_hash(open_remote_ref(remote, ref, "r"), hash);
}
void set_remote_ref(const char *remote, const char *ref, const object_hash_t hash) {
    FILE *file = open_remote_ref(remote, ref, "w");
    if (file == NULL) {
        fprintf(stderr, "Failed to write ref %s on remote %s\n", ref, remote);
        assert(false);
    }
    write_hash(file, hash);
}

linked_list_t *list_branch_refs(void) {
    linked_list_t *refs = init_linked_list();
    DIR *refs_dir = opendir(BRANCH_REFS_DIR);
    assert(refs_dir != NULL);
    struct dirent *entry;
    while ((entry = readdir(refs_dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        char *name_copy = strdup(entry->d_name);
        assert(name_copy != NULL);
        list_push_back(refs, name_copy);
    }
    closedir(refs_dir);
    return refs;
}
