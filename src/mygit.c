#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "add.h"
#include "commit.h"
#include "checkout.h"
#include "fetch.h"
#include "log.h"
#include "push.h"
#include "status.h"

typedef enum {
    ADD,
    CHECKOUT,
    COMMIT,
    FETCH,
    HELP,
    LOG,
    PUSH,
    STATUS,
    INVALID_SUBCOMMAND
} subcommand_t;

const char GIT_DIR[] = "/.git";
const char ADD_CMD[] = "add";
const char CHECKOUT_CMD[] = "checkout";
const char COMMIT_CMD[] = "commit";
const char FETCH_CMD[] = "fetch";
const char HELP_CMD[] = "help";
const char LOG_CMD[] = "log";
const char PUSH_CMD[] = "push";
const char STATUS_CMD[] = "status";

const char USAGE_STRING[] = "Usage:\n"
    "\tmygit add ...\n"
    "\tmygit checkout ...\n"
    "\tmygit commit ...\n"
    "\tmygit fetch ...\n"
    "\tmygit help [COMMAND]\n"
    "\tmygit log ...\n"
    "\tmygit push ...\n";
const char ADD_USAGE_STRING[] = "Usage:\n"
    "\tmygit add [FILES ...]\n\n"
    "\tStages files for commit\n"
    "\tFILES: files or directories to stage for commit\n";
const char CHECKOUT_USAGE_STRING[] = "Usage:\n"
    "\tmygit checkout [-b] REF\n\n"
    "\tChecks out the files at a given commit\n"
    "\tREF: either a commit hash or a branch name\n"
    "\t-b: create a new branch\n";
const char COMMIT_USAGE_STRING[] = "Usage:\n"
    "\tmygit commit -m MESSAGE\n\n"
    "\tMakes a commit to the current branch\n"
    "\tMESSAGE: the commit message\n";
const char FETCH_USAGE_STRING[] = "Usage:\n"
    "\tmygit fetch [REMOTES ...]\n\n"
    "\tFetches all branches from a set of git remotes\n"
    "\tREMOTES: the names of the remotes to fetch. If omitted, fetches all remotes.\n";
const char HELP_USAGE_STRING[] = "Usage:\n"
    "\tmygit help [COMMAND]\n\n"
    "\tPrints out usage information for mygit or a mygit subcommand\n"
    "\tCOMMAND: the mygit subcommand, e.g. 'commit'\n";
const char LOG_USAGE_STRING[] = "Usage:\n"
    "\tmygit log [REF]\n\n"
    "\tLogs the commit history up to a given commit\n"
    "\tREF: the commit hash or branch name whose history to log."
        " If omitted, logs the current commit.\n";
const char PUSH_USAGE_STRING[] = "Usage:\n"
    "\tmygit push [-u REMOTE] [BRANCHES ...]\n\n"
    "\tPushes branches to their corresponding remotes\n"
    "\t-u REMOTE: configures the branches to be pushed to the given remote\n"
    "\tBRANCHES: the branch names to push. If omitted, pushes all branches.\n";
const char STATUS_USAGE_STRING[] = "Usage:\n"
    "\tmygit status\n\n"
    "\tDisplays the state of the staging area\n";
const char *USAGE_STRINGS[] = {
    ADD_USAGE_STRING,
    CHECKOUT_USAGE_STRING,
    COMMIT_USAGE_STRING,
    FETCH_USAGE_STRING,
    HELP_USAGE_STRING,
    LOG_USAGE_STRING,
    PUSH_USAGE_STRING,
    STATUS_USAGE_STRING
};

subcommand_t get_subcommand(char *cmd) {
    if (strcmp(cmd, ADD_CMD) == 0) return ADD;
    if (strcmp(cmd, CHECKOUT_CMD) == 0) return CHECKOUT;
    if (strcmp(cmd, COMMIT_CMD) == 0) return COMMIT;
    if (strcmp(cmd, FETCH_CMD) == 0) return FETCH;
    if (strcmp(cmd, HELP_CMD) == 0) return HELP;
    if (strcmp(cmd, LOG_CMD) == 0) return LOG;
    if (strcmp(cmd, PUSH_CMD) == 0) return PUSH;
    if (strcmp(cmd, STATUS_CMD) == 0) return STATUS;
    return INVALID_SUBCOMMAND;
}

/*
 * scans up directory tree until a directory containing a .git
 * directory is encountered
 *
 * fatal error if no directory is found or can't cd
 */
void cd_to_git_dir(void) {
    char *cwd = getcwd(NULL, 0);
    assert(cwd != NULL);
    for (char *dir = cwd; *dir != '\0'; *strrchr(dir, '/') = '\0') {
        char git_dir[strlen(dir) + strlen(GIT_DIR) + 1];
        strcpy(git_dir, dir);
        strcat(git_dir, GIT_DIR);

        /* check if directory exists */
        struct stat sb;
        if (stat(git_dir, &sb) == 0 && S_ISDIR(sb.st_mode)) {
            int result = chdir(dir);
            assert(result == 0);
            free(cwd);
            return;
        }
    }
    free(cwd);

    fprintf(stderr, "fatal: no .git directory found\n");
    exit(1);
}

typedef void (*run_subcommand_t)(size_t argc, char *argv[]);

void run_add(size_t argc, char *argv[]) {
    cd_to_git_dir();
    add_files((const char **) argv, argc);
}

void run_checkout(size_t argc, char *argv[]) {
    bool make_branch = argc > 1;
    if (make_branch) {
        if (strcmp(argv[0], "-b") != 0) {
            fprintf(stderr, CHECKOUT_USAGE_STRING);
            exit(1);
        }
        argc--;
        argv++;
    }
    if (argc != 1) {
        fprintf(stderr, CHECKOUT_USAGE_STRING);
        exit(1);
    }

    cd_to_git_dir();
    checkout(argv[0], make_branch);
}

void run_commit(size_t argc, char *argv[]) {
    if (argc != 2 || strcmp(argv[0], "-m") != 0) {
        fprintf(stderr, COMMIT_USAGE_STRING);
        exit(1);
    }

    cd_to_git_dir();
    commit(argv[1]);
}

void run_fetch(size_t argc, char *argv[]) {
    cd_to_git_dir();
    if (argc == 0) fetch_all();
    else {
        for (size_t i = 0; i < argc; i++) fetch(argv[i]);
    }
}

void run_help(size_t argc, char *argv[]) {
    if (argc == 0) printf(USAGE_STRING);
    else if (argc == 1) {
        subcommand_t subcommand = get_subcommand(argv[0]);
        if (subcommand == INVALID_SUBCOMMAND) {
            fprintf(stderr, HELP_USAGE_STRING);
            exit(1);
        }

        printf("%s", USAGE_STRINGS[subcommand]);
    }
    else {
        fprintf(stderr, HELP_USAGE_STRING);
        exit(1);
    }
}

void run_log(size_t argc, char *argv[]) {
    if (argc > 1) {
        fprintf(stderr, LOG_USAGE_STRING);
        exit(1);
    }

    cd_to_git_dir();
    mygit_log(argc == 0 ? NULL : argv[0]);
}

void run_push(size_t argc, char *argv[]) {
    char *set_remote;
    if (argc > 0 && strcmp(argv[0], "-u") == 0) {
        if (argc <= 2) {
            // At least 1 branch must be specified
            fprintf(stderr, PUSH_USAGE_STRING);
            exit(1);
        }
        set_remote = argv[1];
        argc -= 2;
        argv += 2;
    }
    else set_remote = NULL;

    cd_to_git_dir();
    push(argc, argc == 0 ? NULL : (const char **) argv, set_remote);
}

void run_status(size_t argc, char *argv[]) {
    (void) argc;
    (void) argv;
    cd_to_git_dir();
    status();
}

run_subcommand_t RUN_SUBCOMMANDS[] = {
    run_add,
    run_checkout,
    run_commit,
    run_fetch,
    run_help,
    run_log,
    run_push,
    run_status
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, USAGE_STRING);
        return 1;
    }
    subcommand_t subcommand = get_subcommand(argv[1]);
    if (subcommand == INVALID_SUBCOMMAND) {
        fprintf(stderr, USAGE_STRING);
        return 1;
    }

    RUN_SUBCOMMANDS[subcommand](argc - 2, argv + 2);
}
