#ifndef PUSH_H
#define PUSH_H

#include <stddef.h>

/**
 * Pushes the specified branch names to their respective remotes.
 * If `branch_count` is 0 and `branches` is NULL, pushes all local branches.
 *
 * @param branch_count the number of branches to push
 * @param branches the names of all the branches to push
 * @param set_remote if non-NULL, the remote to configure these branches to be pushed to
 */
void push(size_t branch_count, const char **branches, const char *set_remote);

#endif // #ifndef PUSH_H
