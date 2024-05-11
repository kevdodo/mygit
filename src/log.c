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

void print_commit(commit_t *commit) {
    // printf("commit %s\n", );
    // printf("Merge:");
    // for (int i = 0; commit->parents[i] !=NULL; i++) {
    //     printf(" %s", oid_to_hex(commit->parents[i]));
    // }
    // printf("\n");
    // printf("Author: %s <%s>\n", commit->author.name, commit->author.email);

    // char date_str[30];
    // strftime(date_str, sizeof(date_str), "%a %b %d %H:%M:%S %Y %z", localtime(&commit->author.when.time));
    // printf("Date: %s\n", date_str);

    // printf("\n%s\n\n", commit->message);
}


void mygit_log(const char *ref) {

}
