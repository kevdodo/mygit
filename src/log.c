#include "log.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "object_io.h"
#include "ref_io.h"

#include <stdio.h>
#include "commit.h"

#include "config_io.h"

void print_commit(object_hash_t hash) {
    commit_t *commit = read_commit(hash);

    while (commit != NULL){
        object_hash_t *parent_hashes = commit->parent_hashes;
        printf("commit %s\n", hash);

        printf("Author: %s \n", commit->author);

        // Convert the time_t object to a struct tm object
        struct tm *timeinfo = localtime(&(commit->author_time));

        // Create a buffer to hold the formatted date
        char buff[80];
        // Format the date
        strftime(buff, 80, "%a %b %d %H:%M:%S %Y %z", timeinfo);
        printf("Date: %s\n", buff);
        printf("parent hash: %s\n", parent_hashes[0]);

        printf("%s\n\n", commit->message);

        commit = read_commit(parent_hashes[0]);
        hash = parent_hashes[0];

    }
}

void mygit_log(const char *ref) {
    char *commit_hash = malloc(sizeof(char) * (HASH_STRING_LENGTH + 1));

    if (ref == NULL){
        bool detached;
        char *head = read_head_file(&detached);

        bool found = head_to_hash(head, detached, commit_hash);
        if (!found){
            free(commit_hash);
            commit_hash = NULL;
            printf("No commits");
            exit(1);
        }
    } else {
        object_hash_t hash;
        bool found = get_branch_ref(ref, hash);
        if (!found){
            printf("Not a valid hash");
            exit(1);
        }
        print_commit(hash);
    }


}
