#include "object_io.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <openssl/sha.h>
#define ZLIB_CONST
#include <zlib.h>
#include "util.h"

const char OBJECT_DIR[] = ".git/objects/";
const size_t HASH_DIRECTORY_LENGTH = 2;
const size_t HASH_FILE_LENGTH = HASH_STRING_LENGTH - HASH_DIRECTORY_LENGTH;

const char BLOB_TYPE[] = "blob";
const char COMMIT_TYPE[] = "commit";
const char TAG_TYPE[] = "tag";
const char TREE_TYPE[] = "tree";

// Object files start with "OBJECT_TYPE LENGTH\0".
// "commit" is the longest type and 20 base-10 digits for the length should be plenty.
const size_t MAX_OBJECT_HEADER_LENGTH = sizeof(COMMIT_TYPE) + 20 + 1;
const size_t MAX_TIME_STRING_LENGTH = 30; // e.g. "1590023943 -0700"
const size_t MAX_MODE_STRING_LENGTH = 6; // e.g. "100644"
const size_t INFLATE_CHUNK_SIZE = 64 << 10; // 64 KB
const size_t DEFLATE_CHUNK_SIZE = 128 << 10; // 128 KB

const char TREE_LINE[] = "tree ";
const char PARENT_LINE[] = "parent ";
const char AUTHOR_LINE[] = "author ";
const char COMMITTER_LINE[] = "committer ";

const size_t MAX_COPY_OFFSET_BYTES = 4;
const size_t MAX_COPY_SIZE_BYTES = 3;

FILE *open_object(const object_hash_t hash, const char *mode) {
    assert(strlen(hash) == HASH_STRING_LENGTH);
    char filename[strlen(OBJECT_DIR) + HASH_DIRECTORY_LENGTH + 1 + HASH_FILE_LENGTH + 1];
    strcpy(filename, OBJECT_DIR);
    strncat(filename, hash, HASH_DIRECTORY_LENGTH);
    if (mode[0] == 'w') mkdir(filename, 0755);
    strcat(filename, "/");
    strcat(filename, hash + HASH_DIRECTORY_LENGTH);
    return fopen(filename, mode);
}

void *read_object(const object_hash_t hash, object_type_t *type, size_t *length) {
    FILE *file = open_object(hash, "r");
    if (file == NULL) {
        fprintf(stderr, "Failed to open object: %s\n", hash);
        assert(false);
    }
    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    int result = inflateInit(&stream);
    assert(result == Z_OK);

    // Read the object header (and likely some of the following bytes)
    char header[MAX_OBJECT_HEADER_LENGTH];
    stream.next_out = (void *) header;
    stream.avail_out = MAX_OBJECT_HEADER_LENGTH;
    uint8_t chunk[INFLATE_CHUNK_SIZE];
    while (stream.avail_out > 0) { // fill up the header
        stream.next_in = chunk;
        stream.avail_in = fread(chunk, 1, INFLATE_CHUNK_SIZE, file);
        if (stream.avail_in == 0 && feof(file)) break;

        result = inflate(&stream, Z_NO_FLUSH);
        assert(result == Z_OK || result == Z_STREAM_END);
    }
    // Ensure the header is null-terminated; otherwise we will cause buffer overflows
    if (memchr(header, '\0', stream.total_out) == NULL) {
        fprintf(stderr, "Object header not found\n");
        assert(false);
    }
    char *type_end = strchr(header, ' ');
    assert(type_end != NULL);
    *type_end = '\0';
    if (!strcmp(header, BLOB_TYPE)) *type = BLOB;
    else if (!strcmp(header, COMMIT_TYPE)) *type = COMMIT;
    else if (!strcmp(header, TREE_TYPE)) *type = TREE;
    else {
        fprintf(stderr, "Unrecognized object type: %s\n", header);
        assert(false);
    }
    *length = 0;
    char *length_digit = type_end + 1;
    while (*length_digit) {
        *length = 10 * *length + from_decimal(*length_digit);
        length_digit++;
    }
    uint8_t *inflated = malloc(*length);
    assert(inflated != NULL);
    // Copy the extra bytes read to the start of the inflated data
    uint8_t *data_start = (uint8_t *) (length_digit + 1);
    size_t extra_header_bytes = stream.next_out - data_start;
    memcpy(inflated, data_start, extra_header_bytes);
    stream.next_out = inflated + extra_header_bytes;
    stream.avail_out = *length - extra_header_bytes;
    while (result != Z_STREAM_END) {
        if (stream.avail_in == 0) {
            stream.next_in = chunk;
            stream.avail_in = fread(chunk, 1, INFLATE_CHUNK_SIZE, file);
        }
        result = inflate(&stream, Z_NO_FLUSH);
        assert(result == Z_OK || result == Z_STREAM_END);
    }
    assert(stream.avail_out == 0);

    result = inflateEnd(&stream);
    assert(result == Z_OK);
    fclose(file);
    return inflated;
}

void read_name_and_time(
    char **line,
    size_t *length,
    const char *type,
    char **name,
    time_t *time
) {
    size_t type_length = strlen(type);
    assert(*length > type_length && starts_with(*line, type));
    *line += type_length;
    *length -= type_length;
    char *email_end = memchr(*line, '>', *length);
    assert(email_end != NULL);
    size_t name_length = email_end + 1 - *line;
    *name = malloc(sizeof(char[name_length + 1]));
    assert(*name != NULL);
    memcpy(*name, *line, sizeof(char[name_length]));
    (*name)[name_length] = '\0';
    *line += name_length;
    *length -= name_length;
    assert(*length > 0 && **line == ' ');
    (*line)++;
    (*length)--;
    *time = 0;
    while (true) {
        assert(*length > 0);
        if (**line == ' ') break;

        *time = *time * 10 + from_decimal(**line);
        (*line)++;
        (*length)--;
    }
    char *line_end = memchr(*line, '\n', *length);
    assert(line_end != NULL);
    size_t timezone_length = line_end + 1 - *line;
    *line += timezone_length;
    *length -= timezone_length;
}

commit_t *read_commit(const object_hash_t hash) {
    object_type_t type;
    size_t length;
    char *data = read_object(hash, &type, &length);
    length /= sizeof(char);
    assert(type == COMMIT);
    commit_t *commit = malloc(sizeof(*commit));
    assert(commit != NULL);

    // Read tree line
    char *line = data;
    assert(length > strlen(TREE_LINE) && starts_with(line, TREE_LINE));
    line += strlen(TREE_LINE);
    length -= strlen(TREE_LINE);
    assert(length > HASH_STRING_LENGTH);
    memcpy(commit->tree_hash, line, sizeof(char[HASH_STRING_LENGTH]));
    commit->tree_hash[HASH_STRING_LENGTH] = '\0';
    line += HASH_STRING_LENGTH + 1;
    length -= HASH_STRING_LENGTH + 1;
    assert(line[-1] == '\n');

    // Read parents lines
    commit->parents = 0;
    char *parent_line = line;
    size_t parent_length = strlen(PARENT_LINE);
    while (length > parent_length && starts_with(line, PARENT_LINE)) {
        line += parent_length;
        length -= parent_length;
        commit->parents++;
        assert(length > HASH_STRING_LENGTH);
        line += HASH_STRING_LENGTH + 1;
        length -= HASH_STRING_LENGTH + 1;
        assert(line[-1] == '\n');
    }
    commit->parent_hashes = malloc(sizeof(object_hash_t[commit->parents]));
    assert(commit->parent_hashes != NULL);
    for (size_t i = 0; i < commit->parents; i++) {
        parent_line += strlen(PARENT_LINE);
        memcpy(commit->parent_hashes[i], parent_line, sizeof(char[HASH_STRING_LENGTH]));
        commit->parent_hashes[i][HASH_STRING_LENGTH] = '\0';
        parent_line += HASH_STRING_LENGTH + 1;
    }
    assert(parent_line == line);

    // Read author and committer lines
    read_name_and_time(&line, &length, AUTHOR_LINE, &commit->author, &commit->author_time);
    read_name_and_time(&line, &length, COMMITTER_LINE, &commit->committer, &commit->commit_time);

    // The rest of the file is the commit message
    assert(length > 0 && *line == '\n');
    line++;
    length--;
    if (length > 0 && line[length - 1] == '\n') {
        length--; // trim the trailing newline if there is one
    }
    commit->message = malloc(sizeof(char[length + 1]));
    assert(commit->message != NULL);
    memcpy(commit->message, line, sizeof(char[length]));
    commit->message[length] = 0;

    free(data);
    return commit;
}

void get_time_string(time_t time, char time_string[MAX_TIME_STRING_LENGTH]) {
    // Format the author/committer time as epoch time + local timezone offset
    size_t printed = snprintf(time_string, MAX_TIME_STRING_LENGTH, "%ld ", time);
    assert(printed < MAX_TIME_STRING_LENGTH);
    struct tm local_time;
    localtime_r(&time, &local_time);
    size_t remaining_length = MAX_TIME_STRING_LENGTH - printed;
    size_t timezone_length =
        strftime(time_string + printed, remaining_length, "%z", &local_time);
    assert(timezone_length > 0);
}

void free_commit(commit_t *commit) {
    free(commit->parent_hashes);
    free(commit->author);
    free(commit->committer);
    free(commit->message);
    free(commit);
}

tree_t *read_tree(const object_hash_t hash) {
    object_type_t type;
    size_t length;
    uint8_t *data = read_object(hash, &type, &length);
    assert(type == TREE);

    // Count the number of entries
    size_t entries = 0;
    uint8_t *offset = data;
    while (length > 0) {
        while (true) {
            assert(length > 0);
            if (*offset == ' ') break;

            offset++;
            length--;
        }
        offset++;
        length--;
        while (true) {
            assert(length > 0);
            if (*offset == '\0') break;

            offset++;
            length--;
        }
        assert(length > HASH_BYTES);
        offset += 1 + HASH_BYTES;
        length -= 1 + HASH_BYTES;
        entries++;
    }

    tree_t *tree = malloc(sizeof(*tree));
    assert(tree != NULL);
    tree->entry_count = entries;
    tree->entries = malloc(sizeof(tree_entry_t[entries]));
    assert(tree->entries != NULL);
    offset = data;
    for (size_t i = 0; i < entries; i++) {
        tree->entries[i].mode = 0;
        while (*offset != ' ') {
            tree->entries[i].mode = tree->entries[i].mode << 3 | from_octal(*offset);
            offset++;
        }
        offset++;
        size_t name_size = sizeof(char[strlen((char *) offset) + 1]);
        tree->entries[i].name = malloc(name_size);
        assert(tree->entries[i].name != NULL);
        memcpy(tree->entries[i].name, offset, name_size);
        offset += name_size;
        hash_to_hex(offset, tree->entries[i].hash);
        offset += HASH_BYTES;
    }
    free(data);
    return tree;
}

void free_tree(tree_t *tree) {
    for (size_t i = 0; i < tree->entry_count; i++) free(tree->entries[i].name);
    free(tree->entries);
    free(tree);
}

blob_t *read_blob(const object_hash_t hash) {
    blob_t *blob = malloc(sizeof(*blob));
    assert(blob != NULL);
    object_type_t type;
    blob->contents = read_object(hash, &type, &blob->length);
    assert(type == BLOB);
    return blob;
}

void free_blob(blob_t *blob) {
    free(blob->contents);
    free(blob);
}

size_t decode_varint(const uint8_t **data, size_t *length) {
    size_t value = 0;
    size_t bits_read = 0;
    bool more_bytes = true;
    while (more_bytes) {
        assert(*length > 0);
        uint8_t byte = *((*data)++);
        (*length)--;
        value |= (byte & 0b1111111) << bits_read;
        bits_read += 7;
        more_bytes = byte >> 7;
    }
    return value;
}

size_t get_header(object_type_t type, size_t length, char *header) {
    // Refuse to write object types that shouldn't be saved
    const char *type_string =
        type == BLOB ? BLOB_TYPE :
        type == COMMIT ? COMMIT_TYPE :
        type == TREE ? TREE_TYPE :
        NULL;
    if (type_string == NULL) {
        fprintf(stderr, "Invalid object type: %d\n", type);
        assert(false);
    }

    size_t printed =
        snprintf(header, MAX_OBJECT_HEADER_LENGTH, "%s %zu", type_string, length);
    assert(printed < MAX_OBJECT_HEADER_LENGTH);
    return sizeof(char[printed + 1]);
}

void get_object_hash(
    object_type_t type,
    const void *contents,
    size_t length,
    object_hash_t hash
) {
    char header[MAX_OBJECT_HEADER_LENGTH];
    size_t header_length = get_header(type, length, header);

    SHA_CTX sha1;
    SHA1_Init(&sha1);
    SHA1_Update(&sha1, header, header_length);
    SHA1_Update(&sha1, contents, length);
    uint8_t hash_bytes[HASH_BYTES];
    SHA1_Final(hash_bytes, &sha1);
    hash_to_hex(hash_bytes, hash);
}

void write_object(
    object_type_t type,
    const void *contents,
    size_t length,
    object_hash_t hash
) {
    char header[MAX_OBJECT_HEADER_LENGTH];
    size_t header_length = get_header(type, length, header);

    get_object_hash(type, contents, length, hash);

    // Deflate file contents to disk
    FILE *file = open_object(hash, "wx");
    if (file == NULL) { // file already exists
        // TODO: should probably check for a hash collision
        return;
    }

    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    int result = deflateInit(&stream, Z_DEFAULT_COMPRESSION);
    assert(result == Z_OK);

    stream.next_in = (uint8_t *) header;
    stream.avail_in = header_length;
    while (stream.avail_in > 0) {
        uint8_t write_buf[DEFLATE_CHUNK_SIZE];
        stream.next_out = write_buf;
        stream.avail_out = DEFLATE_CHUNK_SIZE;
        result = deflate(&stream, Z_NO_FLUSH);
        assert(result == Z_OK);
        size_t out_bytes = stream.next_out - write_buf;
        size_t written = fwrite(write_buf, 1, out_bytes, file);
        assert(written == out_bytes);
    }
    stream.next_in = contents;
    stream.avail_in = length;
    while (stream.avail_in > 0) {
        uint8_t write_buf[DEFLATE_CHUNK_SIZE];
        stream.next_out = write_buf;
        stream.avail_out = DEFLATE_CHUNK_SIZE;
        result = deflate(&stream, Z_NO_FLUSH);
        assert(result == Z_OK);
        size_t out_bytes = stream.next_out - write_buf;
        size_t written = fwrite(write_buf, 1, out_bytes, file);
        assert(written == out_bytes);
    }
    while (result != Z_STREAM_END) {
        uint8_t write_buf[DEFLATE_CHUNK_SIZE];
        stream.next_out = write_buf;
        stream.avail_out = DEFLATE_CHUNK_SIZE;
        result = deflate(&stream, Z_FINISH);
        assert(result == Z_OK || result == Z_STREAM_END);
        size_t out_bytes = stream.next_out - write_buf;
        size_t written = fwrite(write_buf, 1, out_bytes, file);
        assert(written == out_bytes);
    }
    result = deflateEnd(&stream);
    assert(result == Z_OK);
    fclose(file);
}

size_t read_partial_bytes(
    const uint8_t **data,
    size_t *length,
    uint8_t *instruction,
    uint8_t bytes
) {
    size_t value = 0;
    for (uint8_t i = 0; i < bytes; i++) {
        if (*instruction & 1) {
            assert(*length > 0);
            value |= *((*data)++) << (i * 8);
            (*length)--;
        }
        *instruction >>= 1;
    }
    return value;
}

void apply_ref_delta(
    const object_hash_t base_hash,
    const uint8_t *data,
    size_t length,
    object_hash_t hash
) {
    object_type_t base_type;
    size_t base_length;
    uint8_t *base_obj = read_object(base_hash, &base_type, &base_length);

    // Delta data contains 2 varints at start - source size and target size
    size_t source_length = decode_varint(&data, &length);
    assert(base_length == source_length);

    uint8_t result_obj[decode_varint(&data, &length)];
    size_t written = 0;

    while (length > 0) {
        uint8_t instruction = *(data++);
        length--;

        size_t size;
        const uint8_t *source;
        // Copy instruction
        if (instruction >> 7) {
            size_t offset =
                read_partial_bytes(&data, &length, &instruction, MAX_COPY_OFFSET_BYTES);
            size = read_partial_bytes(&data, &length, &instruction, MAX_COPY_SIZE_BYTES);
            // 0 size exception
            if (size == 0) size = 0x10000;
            assert(offset + size <= base_length);
            source = base_obj + offset;
        }
        // New data instruction
        else {
            // Check that this is not a reserved instruction
            assert(instruction != 0);

            size = instruction;
            assert(size <= length);
            source = data;
            data += size;
            length -= size;
        }

        assert(written + size <= sizeof(result_obj));
        memcpy(result_obj + written, source, size);
        written += size;
    }
    assert(written == sizeof(result_obj));
    write_object(base_type, result_obj, sizeof(result_obj), hash);
}
