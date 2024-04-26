#include "util.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

uint8_t from_octal(char c) {
    if (!('0' <= c && c <= '7')) {
        fprintf(stderr, "Invalid octal character: %c\n", c);
        assert(false);
    }

    return c - '0';
}
uint8_t from_decimal(char c) {
    if (!('0' <= c && c <= '9')) {
        fprintf(stderr, "Invalid decimal character: %c\n", c);
        assert(false);
    }

    return c - '0';
}
uint8_t from_hex(char c) {
    if ('0' <= c && c <= '9') return c - '0';
    if ('a' <= c && c <= 'f') return 10 + c - 'a';

    fprintf(stderr, "Invalid hex character: '%c'\n", c);
    assert(false);
}

char to_hex(uint8_t value) {
    if (value < 10) return '0' + value;
    if (value < 16) return 'a' + value - 10;

    fprintf(stderr, "Invalid hex value: '%u'\n", value);
    assert(false);
}

void hash_to_hex(const uint8_t hash_bytes[HASH_BYTES], object_hash_t hash_string) {
    for (size_t i = 0; i < HASH_BYTES; i++) {
        hash_string[i * 2] = to_hex(hash_bytes[i] >> 4);
        hash_string[i * 2 + 1] = to_hex(hash_bytes[i] & 0xF);
    }
    hash_string[HASH_STRING_LENGTH] = '\0';
}

void hex_to_hash(const object_hash_t hash_string, uint8_t hash_bytes[HASH_BYTES]) {
    for (size_t i = 0; i < HASH_BYTES; i++) {
        hash_bytes[i] = from_hex(hash_string[i * 2]) << 4 |
                        from_hex(hash_string[i * 2 + 1]);
    }
}

size_t read_be(const uint8_t *bytes, size_t length) {
    size_t value = 0;
    while (length-- > 0) value = value << 8 | *(bytes++);
    return value;
}

void write_be(size_t value, uint8_t *bytes, size_t length) {
    while (length-- > 0) {
        bytes[length] = value;
        value >>= 8;
    }
    // Ensure that the entire value fit in the bytes
    assert(value == 0);
}

bool starts_with(const char *string, const char *prefix) {
    return !strncmp(string, prefix, strlen(prefix));
}

void make_dirs(char *path) {
    char *next = path;
    while ((next = strchr(next, '/')) != NULL) {
        *next = '\0';
        mkdir(path, 0755);
        *next = '/';
        next++;
    }
}
void make_parent_dirs(char *path) {
    char *file_start = strrchr(path, '/');
    if (file_start == NULL) return;

    // we want to include the trailing slash so make_dirs works properly
    // TODO: Not sure why these methods are separate to be honest
    file_start++; // safe, moves to null terminator in worst case
    char old_file_start = *file_start;
    *file_start = '\0';
    make_dirs(path);
    *file_start = old_file_start;
}

size_t get_file_size(FILE *f) {
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    rewind(f);
    return size;
}
