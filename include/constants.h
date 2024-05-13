#ifndef CONSTANTS_H
#define CONSTANTS_H

#define HASH_BYTES 20
#define HASH_STRING_LENGTH (HASH_BYTES * 2)

/**
 * A full object (i.e. blob, commit, tag, or tree) hash,
 * e.g. "028d95a21de1a2351f1e47154e93bf69128e9a12",
 * including the null terminator.
 */
typedef char object_hash_t[HASH_STRING_LENGTH + 1];

#endif // #define CONSTANTS_H
