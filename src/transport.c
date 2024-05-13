#include "transport.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <openssl/sha.h>
#define ZLIB_CONST
#include <zlib.h>
#include "util.h"

const char SSH[] = "ssh";
const char FETCH_COMMAND[] = "git-upload-pack '";
const char PUSH_COMMAND[] = "git-receive-pack '";
const char CLOSE_QUOTE[] = "'";

const size_t PKT_HEX_LENGTH = 4;
const size_t MAX_PKT_DATA_LENGTH = 65516; // requirement documented in protocol-common.txt
const char FLUSH_PKT[] = "0000";

const char ERR[] = "ERR ";
const char WANT[] = "want ";
const char HAVE[] = "have ";
const char ACK[] = "ACK ";
const char NAK[] = "NAK";
const char READY[] = "ready";
const char COMMON[] = "common";
const char DONE[] = "done";
const char UNPACK[] = "unpack ";
const char UNPACK_OK[] = "ok";
const char REF_OK[] = "ok ";
const char REF_NOT_GOOD[] = "ng ";
const object_hash_t ZERO_HASH = "0000000000000000000000000000000000000000";

const char FETCH_CAPABILITIES[] = " side-band-64k multi_ack_detailed";
const char PUSH_CAPABILTIIES[] = "report-status";

const char BRANCH_REF_PREFIX[] = "refs/heads/";

const uint8_t PACK_SIGNATURE[] = {'P', 'A', 'C', 'K'};
const size_t PACK_SIGNATURE_LENGTH = sizeof(PACK_SIGNATURE);
const size_t PACK_VERSION_LENGTH = 4;
const size_t PACK_OBJECT_COUNT_LENGTH = 4;
const size_t PACK_VERSION = 2;
const size_t PUSH_DEFLATE_CHUNK_SIZE = 128 << 10; // 128 KB

typedef enum {
    PACK = 1,
    PROGRESS = 2,
    ERROR = 3
} stream_code_t;

typedef enum {
    // Fetch states
    FETCH_RECEIVING_REFS,
    SENDING_FIRST_WANT,
    SENDING_ADDITIONAL_WANT,
    SENDING_HAVES,
    SENDING_HAVES_READY,
    RECEIVING_PACK,

    // Push states
    PUSH_RECEIVING_REFS,
    SENDING_FIRST_UPDATE,
    SENDING_ADDITIONAL_UPDATE,
    SENDING_PACK_HEADER,
    SENDING_PACK_OBJECTS,
    CHECKING_UPDATES,

    TRANSPORT_DONE
} transport_stage_t;

typedef struct {
    z_stream stream;
    SHA_CTX sha1;
} pack_state_t;

struct transport {
    pid_t ssh_pid;
    int ssh_read_fd, ssh_write_fd;
    transport_stage_t stage;

    // Additional state for pushes
    bool needs_pack;
    pack_state_t *pack_state;
};

// Representing the layout of the argument to a pipe() syscall
typedef struct {
    int read_fd, write_fd;
} pipe_t;

_Noreturn void exec_ssh(
    transport_direction_t direction,
    const char *ssh_login,
    const char *project
) {

    printf("ermmmmmm\n");

    const char *command = direction == FETCH ? FETCH_COMMAND : PUSH_COMMAND;
    // TODO: sanitize project to protect against bash injection?
    char ssh_command[strlen(command) + strlen(project) + strlen(CLOSE_QUOTE) + 1];
    strcpy(ssh_command, command);
    strcat(ssh_command, project);
    strcat(ssh_command, CLOSE_QUOTE);
    char *ssh_extra_arg;
    char **argv;
    if ((ssh_extra_arg = getenv("MYGIT_TESTS_SSH_EXTRA_ARG"))) {
        char* the_argv[] = {(char *) SSH, (char*)ssh_extra_arg, (char *) ssh_login, ssh_command, NULL};
        argv = the_argv;
    } else {
        char* the_argv[] = {(char *) SSH, (char *) ssh_login, ssh_command, NULL};
        argv = the_argv;
    }
    execvp(argv[0], argv);
    fprintf(stderr, "SSH command failed\n");
    assert(false);
}

transport_t *open_transport(transport_direction_t direction, char *url) {
    assert(direction == FETCH || direction == PUSH);
    // We will replace the : between the SSH login
    // and the project name with '\0' in the child process
    char *ssh_login_end = strchr(url, ':');
    if (ssh_login_end == NULL) {
        fprintf(stderr, "Invalid SSH URL: %s\n", url);
        assert(false);
    }

    // Create a bidirectional pipe for the SSH process.
    // The pipes are referred to from the parent process's perspective.
    pipe_t read_pipe, write_pipe;
    int result = pipe((int *) &read_pipe);
    assert(result == 0);
    result = pipe((int *) &write_pipe);
    assert(result == 0);
    
    
    printf("\nurl here %s\n", url);
    printf("direction : %u\n", direction);
    // Fork off a subprocess to run SSH
    pid_t ssh_pid = fork();
    assert(ssh_pid >= 0);
    if (ssh_pid == 0) {
        // The child process closes the ends of the pipes used by the parent
        result = close(read_pipe.read_fd);
        assert(result == 0);
        result = close(write_pipe.write_fd);
        assert(result == 0);
        // Connect the ends of the pipes to stdin and stdout of the SSH process.
        // The stdout pipe will be read from the parent and written to by the child.
        // The stdin pipe will be written to by the parent and read from the child.
        result = dup2(read_pipe.write_fd, STDOUT_FILENO);
        assert(result >= 0);
        result = dup2(write_pipe.read_fd, STDIN_FILENO);
        assert(result >= 0);
        size_t ssh_login_length = ssh_login_end - url;
        char *ssh_login = malloc(sizeof(char[ssh_login_length + 1]));
        assert(ssh_login != NULL);
        memcpy(ssh_login, url, sizeof(char[ssh_login_length]));
        ssh_login[ssh_login_length] = '\0';
        exec_ssh(direction, ssh_login, ssh_login_end + 1);
    }
    else {
        // The parent process closes the ends of the pipes used by the child
        result = close(read_pipe.write_fd);
        assert(result == 0);
        result = close(write_pipe.read_fd);
        assert(result == 0);

        // Create a transport_t struct storing the state of the SSH process
        transport_t *transport = malloc(sizeof(*transport));
        assert(transport != NULL);
        transport->ssh_pid = ssh_pid;
        transport->ssh_read_fd = read_pipe.read_fd;
        transport->ssh_write_fd = write_pipe.write_fd;
        transport->stage = direction == FETCH
            ? FETCH_RECEIVING_REFS
            : PUSH_RECEIVING_REFS;
        transport->needs_pack = false; // only used for push
        return transport;
    }
}

void close_transport(transport_t *transport) {
    assert(transport->stage == TRANSPORT_DONE);
    // Close the input to the SSH process, which should cause it to exit
    int result = close(transport->ssh_write_fd);
    assert(result == 0);
    // Wait for the SSH process to exit
    int exit_status;
    pid_t pid = waitpid(transport->ssh_pid, &exit_status, 0);
    assert(pid == transport->ssh_pid);
    // Close the output from the SSH process
    result = close(transport->ssh_read_fd);
    assert(result == 0);
    free(transport);
}

// Tries to read `length` bytes into `buffer`. Returns whether successful.
bool read_full(int fd, void *buffer, size_t length) {
    // Repeatedly call read() since it may only read part of the buffer each time
    while (length > 0) {
        ssize_t read_length = read(fd, buffer, length);
        if (read_length < 0) return false;

        buffer += read_length;
        length -= read_length;
    }
    return true;
}

void *read_pkt_line(transport_t *transport, size_t *length) {
    char hex_length[PKT_HEX_LENGTH];
    if (!read_full(transport->ssh_read_fd, hex_length, sizeof(hex_length))) {
        fprintf(stderr, "Failed to read pkt-len\n");
        assert(false);
    }
    *length = 0;
    for (size_t i = 0; i < PKT_HEX_LENGTH; i++) {
        *length = *length << 4 | from_hex(hex_length[i]);
    }
    // "flush-pkt" marks the end of the pkt
    if (*length == 0) return NULL;

    assert(*length > PKT_HEX_LENGTH);
    *length -= PKT_HEX_LENGTH;
    char *line = malloc(*length);
    assert(line != NULL);
    if (!read_full(transport->ssh_read_fd, line, *length)) {
        fprintf(stderr, "Failed to read pkt-payload\n");
        assert(false);
    }

    if (*length >= strlen(ERR) && starts_with(line, ERR)) {
        fprintf(stderr, "Server error: %.*s", (int) *length, line);
        assert(false);
    }
    return line;
}

char *read_pkt_line_string(transport_t *transport) {
    size_t *length = malloc(sizeof(size_t));
    char *line = read_pkt_line(transport, length);
    if (line == NULL) return NULL;

    // Replace newline with a null terminator
    assert(line[*length - 1] == '\n');
    line[*length - 1] = '\0';
    return line;
}

void write_pkt_line(transport_t *transport, const void *line, size_t length, bool text) {
    // Text pkt-lines include a trailing newline
    size_t data_length = length + (text ? sizeof(char) : 0);
    assert(data_length <= MAX_PKT_DATA_LENGTH);
    size_t line_length = PKT_HEX_LENGTH + data_length;
    char hex_length[PKT_HEX_LENGTH];
    for (size_t i = 0; i < PKT_HEX_LENGTH; i++) {
        hex_length[i] = to_hex(line_length >> ((PKT_HEX_LENGTH - 1 - i) * 4) & 0xF);
    }
    ssize_t written = write(transport->ssh_write_fd, hex_length, sizeof(hex_length));
    assert(written == sizeof(hex_length));
    written = write(transport->ssh_write_fd, line, length);
    assert(written == (ssize_t) length);
    if (text) {
        char newline = '\n';
        written = write(transport->ssh_write_fd, &newline, sizeof(newline));
        assert(written == sizeof(char));
    }
}
void write_pkt_line_string(transport_t *transport, const char *line) {
    size_t length = strlen(line);
    write_pkt_line(transport, line, sizeof(char[length]), true);
}

void send_flush_pkt(transport_t *transport) {
    size_t length = sizeof(char[strlen(FLUSH_PKT)]);
    ssize_t written = write(transport->ssh_write_fd, FLUSH_PKT, length);
    assert(written == (ssize_t) length);
}

void receive_refs(transport_t *transport, ref_receiver_t receiver, void *aux) {
    bool fetch = transport->stage == FETCH_RECEIVING_REFS;
    assert(fetch || transport->stage == PUSH_RECEIVING_REFS);
    char *line;
    printf("yuhhh\n");

    while ((line = read_pkt_line_string(transport)) != NULL) {
        // We ignore the list of capabilities after the first ref name
        char *hash_end = strchr(line, ' ');
        if (!hash_end) {
            fprintf(stderr, "Expected commit hash and ref name\n");
            assert(false);
        }
        *hash_end = '\0';
        // Ignore the "zero-id" hash, which is sent when there are no refs
        if (strcmp(line, ZERO_HASH)) receiver(hash_end + 1, line, aux);
        free(line);
    }
    transport->stage = fetch ? SENDING_FIRST_WANT : SENDING_FIRST_UPDATE;
}

void send_want(transport_t *transport, const object_hash_t hash) {
    bool first_want = transport->stage == SENDING_FIRST_WANT;
    assert(first_want || transport->stage == SENDING_ADDITIONAL_WANT);
    size_t line_length = strlen(WANT) + strlen(hash) + 1;
    if (first_want) line_length += strlen(FETCH_CAPABILITIES);
    char line[line_length];
    strcpy(line, WANT);
    strcat(line, hash);
    if (first_want) strcat(line, FETCH_CAPABILITIES);
    write_pkt_line_string(transport, line);
    transport->stage = SENDING_ADDITIONAL_WANT;
}

void finish_wants(transport_t *transport) {
    bool first_want = transport->stage == SENDING_FIRST_WANT;
    assert(first_want || transport->stage == SENDING_ADDITIONAL_WANT);
    send_flush_pkt(transport);
    // If no wants are requested, the transport is terminated
    transport->stage = first_want ? TRANSPORT_DONE : SENDING_HAVES;
}

void send_have(transport_t *transport, const object_hash_t hash) {
    assert(transport->stage == SENDING_HAVES);
    char line[strlen(HAVE) + strlen(hash) + 1];
    strcpy(line, HAVE);
    strcat(line, hash);
    write_pkt_line_string(transport, line);
}

bool check_have_acks(transport_t *transport, ack_receiver_t receiver, void *aux) {
    // This shouldn't be called once a "ready" status has been sent
    assert(transport->stage == SENDING_HAVES);
    send_flush_pkt(transport);
    bool ready = false;
    while (true) {
        // Read server ACK lines until a NAK is received
        char *line = read_pkt_line_string(transport);
        assert(line != NULL);
        if (!strcmp(line, NAK)) {
            free(line);
            break;
        }

        // Expect an ACK
        if (!starts_with(line, ACK)) {
            fprintf(stderr, "Invalid ACK/NAK response: %s\n", line);
            assert(false);
        }
        // `line` looks like "ACK 7e47fe2bd8d01d481f44d7af0531bd93d3b21c01 common"
        char *hash = line + strlen(ACK);
        char *status = strchr(hash, ' ');
        if (status == NULL) {
            fprintf(stderr, "ACK status missing: %s\n", line);
            assert(false);
        }
        status++; // skip over the space
        if (!strcmp(status, READY)) ready = true;
        else if (!strcmp(status, COMMON)) {
            status[-1] = '\0'; // null-terminate the hash
            receiver(hash, aux);
        }
        else {
            fprintf(stderr, "Unexpected ACK status: %s\n", status);
            assert(false);
        }
        free(line);
    }
    if (ready) transport->stage = SENDING_HAVES_READY;
    return ready;
}

void finish_haves(transport_t *transport) {
    // TODO: assert that all sent haves have been checked
    assert(transport->stage == SENDING_HAVES || transport->stage == SENDING_HAVES_READY);
    write_pkt_line_string(transport, DONE);
    // Server responds with either an ACK or a NAK, which we can ignore
    char *line = read_pkt_line_string(transport);
    assert(line != NULL);
    assert(starts_with(line, ACK) || !strcmp(line, NAK));
    free(line);
    transport->stage = RECEIVING_PACK;
}

// The state of decoding a PACK file
typedef enum {
    // Reading the 4-byte "PACK" signature
    SIGNATURE,
    // Reading the 32-bit version number
    VERSION,
    // Reading the 32-bit object count
    OBJECT_COUNT,
    // Reading the objects
    OBJECTS,
    // Verifying the SHA1 hash
    VERIFY_SHA,
    // Done processing PACK file
    FINISHED
} pack_read_stage_t;
typedef struct {
    uint8_t bytes[PACK_SIGNATURE_LENGTH];
    size_t bytes_read;
} signature_state_t;
typedef struct {
    uint8_t bytes[PACK_VERSION_LENGTH];
    size_t bytes_read;
} version_state_t;
typedef struct {
    uint8_t bytes[PACK_OBJECT_COUNT_LENGTH];
    size_t bytes_read;
} object_count_state_t;
typedef struct {
    // The number of objects remaining to be read
    size_t objects;
    // Whether reading the type byte; whether reading additional length bytes
    bool reading_type, reading_length;
    // The type of the current object
    object_type_t type;
    // The hash of the base object, for `OBJ_REF_DELTA`
    uint8_t base_hash[HASH_BYTES];
    // The number of hash bytes read so far
    size_t hash_bytes_read;
    // The size of the inflated object
    size_t inflated_length;
    // The number of bits of the inflate length read so far
    uint8_t inflated_length_bits;
    // The heap-allocated inflated data
    uint8_t *inflated;
    // A reusable DEFLATE decompression stream
    z_stream stream;
} objects_state_t;
typedef struct {
    uint8_t bytes[HASH_BYTES];
    size_t bytes_read;
} verify_sha_state_t;
typedef struct {
    // A tagged union of the PACK file read state
    pack_read_stage_t stage;
    union {
        signature_state_t signature_state;
        version_state_t version_state;
        object_count_state_t object_count_state;
        objects_state_t objects_state;
        verify_sha_state_t verify_sha_state;
    };

    // Object callback function and auxiliary parameter
    object_receiver_t receiver;
    void *aux;

    // The current chunk of the PACK file being read.
    // The file can be split arbitrarily into chunks.
    const uint8_t *chunk;
    // The number of bytes remaining in the current chunk
    size_t length;

    // Total bytes of the PACK file read
    size_t bytes_read;
    // Current SHA-1 state of the PACK file
    SHA_CTX sha1;
} pack_read_state_t;

bool read_bytes(
    pack_read_state_t *state,
    uint8_t *destination,
    size_t *bytes_read,
    size_t max_length,
    bool hash
) {
    size_t remaining_bytes = max_length - *bytes_read;
    if (state->length < remaining_bytes) remaining_bytes = state->length;
    memcpy(destination + *bytes_read, state->chunk, remaining_bytes);
    if (hash) {
        state->bytes_read += remaining_bytes;
        SHA1_Update(&state->sha1, state->chunk, remaining_bytes);
    }
    state->chunk += remaining_bytes;
    state->length -= remaining_bytes;
    *bytes_read += remaining_bytes;
    return *bytes_read == max_length;
}
uint8_t read_byte(pack_read_state_t *state) {
    uint8_t byte;
    size_t bytes_read = 0;
    read_bytes(state, &byte, &bytes_read, 1, true);
    assert(bytes_read == 1);
    return byte;
}

/**
 * Parses a chunk of a PACK file. May only consume part of the chunk,
 * so must be called repeatedly until state->length becomes 0.
 * Since there are no guarantees how the PACK file will be chunked up,
 * this is basically a big state machine.
 */
void process_pack_chunk(pack_read_state_t *state) {
    assert(state->length > 0);
    switch (state->stage) {
        case SIGNATURE:
            if (read_bytes(
                state,
                state->signature_state.bytes,
                &state->signature_state.bytes_read,
                PACK_SIGNATURE_LENGTH,
                true
            )) {
                // PACK file should start with "PACK"
                assert(!memcmp(
                    state->signature_state.bytes,
                    PACK_SIGNATURE,
                    PACK_SIGNATURE_LENGTH
                ));
                state->stage++;
                state->version_state.bytes_read = 0;
            }
            break;

        case VERSION:
            if (read_bytes(
                state,
                state->version_state.bytes,
                &state->version_state.bytes_read,
                PACK_VERSION_LENGTH,
                true
            )) {
                // Version should be 2
                size_t version = read_be(state->version_state.bytes, PACK_VERSION_LENGTH);
                if (version != PACK_VERSION) {
                    fprintf(stderr, "Unknown pack version: %zu\n", version);
                    assert(false);
                }
                state->stage++;
                state->object_count_state.bytes_read = 0;
            }
            break;

        case OBJECT_COUNT:
            if (read_bytes(
                state,
                state->object_count_state.bytes,
                &state->object_count_state.bytes_read,
                PACK_OBJECT_COUNT_LENGTH,
                true
            )) {
                // Compute how many objects need to be read
                // and initialize the objects read state
                state->stage++;
                state->objects_state.objects =
                    read_be(state->object_count_state.bytes, PACK_OBJECT_COUNT_LENGTH);
                state->objects_state.reading_type = true;
                state->objects_state.inflated = NULL;
                memset(&state->objects_state.stream, 0, sizeof(z_stream));
                int result = inflateInit(&state->objects_state.stream);
                assert(result == Z_OK);
            }
            break;

        case OBJECTS:
            // If done reading objects, clean up the zlib stream
            if (state->objects_state.objects == 0) {
                int result = inflateEnd(&state->objects_state.stream);
                assert(result == Z_OK);
                state->stage++;
                state->verify_sha_state.bytes_read = 0;
                break;
            }

            if (state->objects_state.reading_type) {
                // The first byte read contains the object type
                // and 4 bits of the inflated length
                uint8_t byte = read_byte(state);
                state->objects_state.reading_type = false;
                state->objects_state.reading_length = byte >> 7;
                object_type_t type = byte >> 4 & 0b111;
                assert( // ensure this object type can be handled
                    type == COMMIT || type == TREE || type == BLOB ||
                    type == OBJ_REF_DELTA
                );
                state->objects_state.type = type;
                state->objects_state.hash_bytes_read = 0; // only used for OBJ_REF_DELTA
                state->objects_state.inflated_length = byte & 0b1111;
                state->objects_state.inflated_length_bits = 4;
            }
            else if (state->objects_state.reading_length) {
                // Subsequent bytes (while the high bit is 1)
                // contain the next 7 bits of the inflated length
                uint8_t byte = read_byte(state);
                state->objects_state.reading_length = byte >> 7;
                state->objects_state.inflated_length |=
                    (byte & 0b1111111) << state->objects_state.inflated_length_bits;
                state->objects_state.inflated_length_bits += 7;
            }
            else {
                if (state->objects_state.type == OBJ_REF_DELTA) {
                    // OBJ_REF_DELTA objects have a 20-byte hash before the DEFLATE data
                    read_bytes(
                        state,
                        state->objects_state.base_hash,
                        &state->objects_state.hash_bytes_read,
                        HASH_BYTES,
                        true
                    );
                    if (state->length == 0) break;
                }

                if (state->objects_state.inflated == NULL) {
                    // Reached the DEFLATE stream, so allocate space for the object
                    state->objects_state.inflated =
                    state->objects_state.stream.next_out =
                        malloc(state->objects_state.inflated_length);
                    assert(state->objects_state.inflated != NULL);
                    state->objects_state.stream.avail_out =
                        state->objects_state.inflated_length;
                }

                // Use zlib to inflate the object data.
                // The DEFLATE format indicates the end of the object data.
                state->objects_state.stream.next_in = state->chunk;
                state->objects_state.stream.avail_in = state->length;
                int result = inflate(&state->objects_state.stream, Z_FINISH);
                // Hash the consumed bytes
                size_t deflated_length =
                    state->objects_state.stream.next_in - state->chunk;
                state->bytes_read += deflated_length;
                SHA1_Update(&state->sha1, state->chunk, deflated_length);
                // Move to the unconsumed remainder of the chunk
                state->chunk = state->objects_state.stream.next_in;
                state->length = state->objects_state.stream.avail_in;
                if (result == Z_STREAM_END) {
                    // Reached the end of the DEFLATE stream.
                    // Check that all inflated_length bytes were read.
                    assert(state->objects_state.stream.avail_out == 0);
                    object_hash_t base_hash;
                    if (state->objects_state.type == OBJ_REF_DELTA) {
                        // If this is an OBJ_REF_DELTA object,
                        // convert the base object hash to hexadecimal
                        hash_to_hex(state->objects_state.base_hash, base_hash);
                    }
                    // Call the callback with the inflated object
                    state->receiver(
                        state->objects_state.type,
                        state->objects_state.type == OBJ_REF_DELTA ? base_hash : NULL,
                        state->objects_state.inflated,
                        state->objects_state.inflated_length,
                        state->aux
                    );
                    // Reset state for next object
                    state->objects_state.objects--;
                    state->objects_state.reading_type = true;
                    state->objects_state.inflated = NULL;
                    result = inflateReset(&state->objects_state.stream);
                    assert(result == Z_OK);
                }
                else {
                    // Didn't finish the DEFLATE stream; the rest is in future chunks.
                    // Result can be Z_BUF_ERROR is some deflated bytes were read,
                    // but not enough to generate more bytes of output.
                    assert(result == Z_OK || result == Z_BUF_ERROR);
                    assert(state->length == 0);
                }
            }
            break;

        case VERIFY_SHA:
            // Compute the SHA-1 hash of the PACK file and check that it matches
            if (read_bytes(
                state,
                state->verify_sha_state.bytes,
                &state->verify_sha_state.bytes_read,
                HASH_BYTES,
                false
            )) {
                uint8_t actual_hash[HASH_BYTES];
                SHA1_Final(actual_hash, &state->sha1);
                if (memcmp(actual_hash, state->verify_sha_state.bytes, HASH_BYTES)) {
                    fprintf(stderr, "SHA1 hash check of pack file failed\n");
                    assert(false);
                }
                state->stage++;
            }
            break;

        default:
            fprintf(stderr, "Invalid pack stage: %d\n", state->stage);
            assert(false);
    }
}

void receive_pack(transport_t *transport, object_receiver_t receiver, void *aux) {
    assert(transport->stage == RECEIVING_PACK);
    pack_read_state_t state = {
        .stage = SIGNATURE,
        .signature_state = {.bytes_read = 0},
        .receiver = receiver,
        .aux = aux,
        .bytes_read = 0
    };
    SHA1_Init(&state.sha1);

    // Read until the flush-pkt after the end of the PACK file
    size_t line_length;
    uint8_t *line;
    while ((line = read_pkt_line(transport, &line_length)) != NULL) {
        stream_code_t stream_code = line[0];
        line_length--;
        uint8_t *data = line + 1;
        if (stream_code == PACK) {
            // Run the full chunk through the PACK file reader
            state.chunk = data;
            state.length = line_length;
            while (state.length > 0) process_pack_chunk(&state);
        }
        else if (stream_code == PROGRESS || stream_code == ERROR) {
            // TIL the protocol actually sends the messages to print to the terminal,
            // e.g. "Counting objects: 100% (3/3), done."
            if (stream_code == ERROR) fprintf(stderr, "Fatal error: ");
            fprintf(stderr, "%.*s", (int) line_length, (char *) data);
        }
        else {
            fprintf(stderr, "Unrecognized stream code: %d\n", stream_code);
            assert(false);
        }

        free(line);
    }

    assert(state.stage == FINISHED);
    transport->stage = TRANSPORT_DONE;
}

void send_update(
    transport_t *transport,
    const char *ref,
    const object_hash_t old_hash,
    const object_hash_t new_hash
) {
    bool first_update = transport->stage == SENDING_FIRST_UPDATE;
    assert(first_update || transport->stage == SENDING_ADDITIONAL_UPDATE);
    size_t line_length =
        HASH_STRING_LENGTH + 1 + HASH_STRING_LENGTH + 1 + strlen(ref) + 1;
    if (first_update) line_length += strlen(PUSH_CAPABILTIIES) + 1;
    // An old hash of 0 indicates a creation; a new hash of 0 indicates a deletion
    char line[line_length];
    strcpy(line, old_hash == NULL ? ZERO_HASH : old_hash);
    strcat(line, " ");
    strcat(line, new_hash == NULL ? ZERO_HASH : new_hash);
    strcat(line, " ");
    strcat(line, ref);
    if (first_update) strcpy(line + strlen(line) + 1, PUSH_CAPABILTIIES);
    // Can't use write_pkt_line_string() since there is a '\0' in the string
    write_pkt_line(transport, line, sizeof(char[line_length - 1]), true);
    transport->stage = SENDING_ADDITIONAL_UPDATE;
    if (new_hash != NULL) transport->needs_pack = true;
}

void finish_updates(transport_t *transport) {
    bool first_update = transport->stage == SENDING_FIRST_UPDATE;
    assert(first_update || transport->stage == SENDING_ADDITIONAL_UPDATE);
    send_flush_pkt(transport);
    transport->stage = first_update
        ? TRANSPORT_DONE
        : transport->needs_pack ? SENDING_PACK_HEADER : CHECKING_UPDATES;
}

void write_pack(transport_t *transport, const uint8_t *data, size_t length) {
    pack_state_t *pack_state = transport->pack_state;
    ssize_t written = write(transport->ssh_write_fd, data, length);
    assert(written == (ssize_t) length);
    SHA1_Update(&pack_state->sha1, data, length);
}

void start_pack(transport_t *transport, size_t object_count) {
    assert(transport->stage == SENDING_PACK_HEADER);
    transport->pack_state = malloc(sizeof(pack_state_t));
    assert(transport->pack_state != NULL);
    SHA1_Init(&transport->pack_state->sha1);
    write_pack(transport, PACK_SIGNATURE, PACK_SIGNATURE_LENGTH);
    uint8_t version_bytes[PACK_SIGNATURE_LENGTH];
    write_be(PACK_VERSION, version_bytes, PACK_VERSION_LENGTH);
    write_pack(transport, version_bytes, PACK_VERSION_LENGTH);
    uint8_t object_count_bytes[PACK_OBJECT_COUNT_LENGTH];
    write_be(object_count, object_count_bytes, PACK_OBJECT_COUNT_LENGTH);
    write_pack(transport, object_count_bytes, PACK_OBJECT_COUNT_LENGTH);

    memset(&transport->pack_state->stream, 0, sizeof(z_stream));
    int result = deflateInit(&transport->pack_state->stream, Z_DEFAULT_COMPRESSION);
    assert(result == Z_OK);
    transport->stage = SENDING_PACK_OBJECTS;
}

void send_pack_object(
    transport_t *transport,
    object_type_t type,
    const uint8_t *contents,
    size_t length
) {
    // TODO: produce OBJ_REF_DELTA for blobs and trees
    assert(type == COMMIT || type == TREE || type == BLOB);
    assert(transport->stage == SENDING_PACK_OBJECTS);

    // The first byte contains the type and 4 bits of the length
    uint8_t byte = type << 4 | (length & 0b1111);
    size_t remaining_length = length >> 4;
    // The next bytes contain 7 bits of the length each
    while (true) {
        bool more_bytes = remaining_length > 0;
        byte |= more_bytes << 7;
        write_pack(transport, &byte, 1);
        if (!more_bytes) break;

        byte = remaining_length & 0b1111111;
        remaining_length >>= 7;
    }

    // Compress the object to a DEFLATE stream
    uint8_t deflate_chunk[PUSH_DEFLATE_CHUNK_SIZE];
    z_stream *stream = &transport->pack_state->stream;
    stream->next_in = contents;
    stream->avail_in = length;
    int result;
    while (stream->avail_in > 0) {
        stream->next_out = deflate_chunk;
        stream->avail_out = PUSH_DEFLATE_CHUNK_SIZE;
        result = deflate(stream, Z_NO_FLUSH);
        assert(result == Z_OK);
        write_pack(transport, deflate_chunk, stream->next_out - deflate_chunk);
    }
    while (result != Z_STREAM_END) {
        stream->next_out = deflate_chunk;
        stream->avail_out = PUSH_DEFLATE_CHUNK_SIZE;
        result = deflate(stream, Z_FINISH);
        assert(result == Z_OK || result == Z_STREAM_END);
        write_pack(transport, deflate_chunk, stream->next_out - deflate_chunk);
    }
    result = deflateReset(stream);
    assert(result == Z_OK);
}

void finish_pack(transport_t *transport) {
    assert(transport->stage == SENDING_PACK_OBJECTS);
    // Send hash to terminate PACK file
    pack_state_t *pack_state = transport->pack_state;
    uint8_t hash[HASH_BYTES];
    SHA1_Final(hash, &pack_state->sha1);
    write_pack(transport, hash, HASH_BYTES);
    // Close pack state
    int result = deflateEnd(&pack_state->stream);
    assert(result == Z_OK);
    free(pack_state);

    // Check that unpack was successful
    char *line = read_pkt_line_string(transport);
    assert(line != NULL);
    assert(starts_with(line, UNPACK));
    char *unpack_message = line + strlen(UNPACK);
    if (strcmp(unpack_message, UNPACK_OK)) {
        fprintf(stderr, "Unpack error: %s\n", unpack_message);
        assert(false);
    }
    free(line);
    transport->stage = CHECKING_UPDATES;
}

void check_updates(transport_t *transport, updated_ref_receiver_t receiver, void *aux) {
    assert(transport->stage == CHECKING_UPDATES);
    char *line;
    while ((line = read_pkt_line_string(transport)) != NULL) {
        bool success = starts_with(line, REF_OK);
        assert(success || starts_with(line, REF_NOT_GOOD));
        char *ref = line + strlen(success ? REF_OK : REF_NOT_GOOD);
        if (success) receiver(ref, aux);
        else {
            char *ref_end = strchr(ref, ' ');
            assert(ref_end != NULL);
            *ref_end = '\0';
            fprintf(stderr, "Error updating ref %s: %s\n", ref, ref_end + 1);
        }
        free(line);
    }
    transport->stage = TRANSPORT_DONE;
}
