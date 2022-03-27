/*
 * The git protocol is described at https://github.com/git/git/blob/master/Documentation/technical/pack-protocol.txt#L103.
 * Fetching consists of several stages:
 * - The client opens an SSH connection to the server.
 *   Call open_transport(FETCH, ...) to initiate this SSH connection.
 * - The server sends a list of "refs" it knows about,
 *   i.e. branches and tags and their corresponding commit hashes.
 *   Call receive_refs() to handle each ref received from the server.
 * - The client chooses zero or more of these commit hashes that it wants to fetch.
 *   Call send_want() on each commit hash you want to receive.
 *   Call finish_wants() once the commit hashes have been sent.
 *   If no commit hashes were requested, STOP and call close_transport()!
 * - The client tells the server which commits hashes it already has,
 *   so the server can minimize the amount of data sent.
 *   There are different ways the server can decide the oldest commit it needs to send;
 *   we are using the basic strategy: the server chooses
 *   the first commit hash sent by the client that the server also has.
 *   This means that if you want to fetch the newest `master`, for example,
 *   the client should send the commit hash of `master` locally, then its parent,
 *   and so on, until it reaches a commit hash that the server also has.
 *   Call send_have() to send each local commit hash to the server.
 *   Call check_have_acks() periodically to check whether the server
 *   knows any of the commit hashes sent since the last check_have_acks().
 *   But don't call it after every commit hash, since it waits for the server to respond.
 *   Once the server returns that it is ready (or the client knows no more of its commits
 *   are common with the server), call finish_haves().
 * - The server figures out what needs to be sent to the client and sends a PACK file.
 *   This format is documented at https://github.com/git/git/blob/master/Documentation/technical/pack-format.txt.
 *   The file contains all the new objects (commits, trees, etc.) that the client needs.
 *   Call receive_pack() to handle each object received from the server.
 *
 * Pushing is a bit simpler since the client must already know the state of the remote:
 * - The client opens an SSH connection using open_transport(PUSH, ...)
 * - The server sends its current refs, just like at the start of a fetch.
 *   Call receive_refs() to receive these remote refs.
 *   If any ref that is going to be pushed points to a different commit on the remote
 *   than what the client thinks is on the remote, the push should be aborted.
 *   The client will need to run a fetch and merge in the remote's changes.
 * - The client sends a list of refs it wants the server to update.
 *   An update can create, delete, or change a branch.
 *   Call send_update() for each update and finish_updates() once all updates are sent.
 *   If no updates were requested, STOP and call close_transport()!
 * - The client sends all the new objects that the server needs in a PACK file.
 *   If no branches were requested to be created or changed, do not send a PACK file!
 *   Call start_pack() to send the header of the PACK file.
 *   Call send_pack_object() for each object (commit, tree, or blob) in the PACK file.
 *   Call finish_pack() once all objects have been sent.
 * - The server tells the client which updates were successful.
 *   Call check_updates() to receive each successfully updated ref.
 */

#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "constants.h"

extern const char BRANCH_REF_PREFIX[];

typedef enum {
    FETCH,
    PUSH
} transport_direction_t;

typedef struct transport transport_t;

/**
 * Opens a git connection over SSH
 *
 * @param direction whether performing a fetch or a push
 * @param url the SSH login and project, e.g. "git@gitlab.caltech.edu:cs24-19fa/project07"
 * @return a handle to the SSH process that can be used by the functions below
 */
transport_t *open_transport(transport_direction_t direction, char *url);
/** Closes a transport_t obtained from open_transport() */
void close_transport(transport_t *);

/**
 * A callback function called for each ref received from the server.
 * Both `ref` and `hash` may not be used after the callback returns.
 *
 * @param ref the name of the reference, e.g. "refs/heads/master" or "refs/tags/v1.0"
 * @param hash the commit hash, e.g. "00ef4ec3fd981d4e5020634ead3349633dd8ec5a"
 * @param aux an auxiliary value
 */
typedef void (*ref_receiver_t)(char *ref, object_hash_t hash, void *aux);

/**
 * Discovers all refs in the remote repository.
 * Calls the callback on each discovered ref.
 * This should be the first operation performed on a RECEIVE transport.
 */
void receive_refs(transport_t *, ref_receiver_t receiver, void *aux);

// FETCH OPERATIONS

void send_want(transport_t *, const object_hash_t hash);
void finish_wants(transport_t *);

/**
 * A callback function called on a commit that the client and server have in common.
 * `hash` may not be used after the callback returns.
 *
 * @param hash the commit hash
 * @param aux an auxiliary value
 */
typedef void (*ack_receiver_t)(object_hash_t hash, void *aux);

void send_have(transport_t *, const object_hash_t hash);
/**
 * Checks with the server which of the haves sent
 * since the last check_have_acks() are also on the server.
 * Calls `receiver` on each common commit hash.
 * Returns whether the server is ready to send the PACK file.
 * Once the server says it is ready, or the client knows no more commits
 * can be common with the server, finish_haves() must be called.
 */
bool check_have_acks(transport_t *, ack_receiver_t receiver, void *aux);
void finish_haves(transport_t *);

/**
 * The types of objects which can be received in a PACK file.
 * See https://github.com/git/git/blob/master/Documentation/technical/pack-format.txt.
 *
 * `OBJ_REF_DELTA` is a special type indicating an object that is constructed
 * from sections of another object. See the link above for the format.
 */
typedef enum {
    COMMIT = 1,
    TREE = 2,
    BLOB = 3,
    OBJ_REF_DELTA = 7
} object_type_t;

/**
 * A callback function called when an object is received from the server
 *
 * @param type the type of object that was received
 * @param base_hash if `type == OBJ_REF_DELTA`, the hash of the base object
 *   this object will be constructed from (see "Deltified representation").
 *   Otherwise, NULL. This may not be used after the callback returns.
 * @param contents the bytes that make up this object, or in the `OBJ_REF_DELTA` case,
 *   the instructions to construct the object from the base object.
 *   This is heap-allocated and should be free()d by the callback.
 * @param length the number of bytes in `contents`
 * @param aux an auxiliary value
 */
typedef void (*object_receiver_t)(
    object_type_t type,
    object_hash_t base_hash,
    uint8_t *contents,
    size_t length,
    void *aux
);

void receive_pack(transport_t *, object_receiver_t receiver, void *aux);

// PUSH OPERATIONS

/**
 * Requests that the server update the given ref (e.g. "refs/heads/master")
 * from an old commit hash to a new one.
 * Set `old_hash` to NULL to create a new ref; set `new_hash` to NULL to delete the ref.
 */
void send_update(
    transport_t *,
    const char *ref,
    const object_hash_t old_hash,
    const object_hash_t new_hash
);
void finish_updates(transport_t *);

void start_pack(transport_t *, size_t object_count);
void send_pack_object(
    transport_t *,
    object_type_t type,
    const uint8_t *contents,
    size_t length
);
void finish_pack(transport_t *);

/**
 * A callback function called when a remote ref is updated successfully.
 * `ref` may not be used after the callback returns.
 *
 * @param ref the ref that was updated, e.g. "refs/heads/master"
 * @param aux an auxiliary value
 */
typedef void (*updated_ref_receiver_t)(char *ref, void *aux);

void check_updates(transport_t *, updated_ref_receiver_t receiver, void *aux);

#endif // #ifndef TRANSPORT_H
