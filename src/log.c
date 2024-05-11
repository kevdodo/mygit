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
        
        if (commit->parents > 1) {
            printf("Merge: ");
            for (size_t i = 0; i < commit->parents; i++) {
                printf("%s ", parent_hashes[i]);
            }
            printf("\n");
        }
        printf("Author: %s \n", commit->author);

        // Convert the time_t object to a struct tm object
        struct tm *timeinfo = localtime(&(commit->author_time));

        // Create a buffer to hold the formatted date
        char buff[80];
        // Format the date
        strftime(buff, 80, "%a %b %d %H:%M:%S %Y %z", timeinfo);
        printf("Date: %s\n\n", buff);

        printf("%s\n\n", commit->message);
        
        // Ensure hash is an object_hash_t type
        object_hash_t hash;

        if (commit->parents > 0){
            object_hash_t temp_hash;
            memcpy(&temp_hash, &(commit->parent_hashes[0]), sizeof(object_hash_t));
            free_commit(commit);
            

            commit = read_commit(temp_hash);
            memcpy(&hash, &temp_hash, sizeof(object_hash_t));
        } else {
            free_commit(commit);
            commit = NULL;
        }
    }
}

void mygit_log(const char *ref) {

    if (ref == NULL){
        bool detached;
        char *head = read_head_file(&detached);

        object_hash_t hash;
        bool found = head_to_hash(head, detached, hash);
        if (!found){
            printf("No commits");
            exit(1);
        }
        print_commit(hash);
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
