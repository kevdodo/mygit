#include "config_io.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linked_list.h"
#include "util.h"

const char CONFIG_PATH[] = ".git/config";
const char GLOBAL_CONFIG_PATH1[] = "/.gitconfig";
const char GLOBAL_CONFIG_PATH2[] = "/.config/git/config";
const char COMMENT_START = '#';
const char SECTION_START = '[',
           SECTION_END = ']';
const char PROPERTY_START = '\t';
const char PROPERTY_SEPARATOR[] = " = ";
const char BRANCH_SECTION_PREFIX[] = "branch \"";
const char REMOTE_SECTION_PREFIX[] = "remote \"";
const char SECTION_SUFFIX[] = "\"";
const char MERGE_KEY[] = "merge";
const char REMOTE_KEY[] = "remote";
const char URL_KEY[] = "url";

FILE *open_config_file(char *mode) {
    FILE *file = fopen(CONFIG_PATH, mode);
    if (file == NULL) {
        fprintf(stderr, "Failed to open config file\n");
        assert(false);
    }
    return file;
}

static char *skip_whitespace(char* line) {
    while (*line && isspace(*line)) line++;
    return line;
}

char *read_config_line(FILE *file) {
    while (true) {
        char *line = NULL;
        size_t line_capacity = 0;
        ssize_t line_length = getline(&line, &line_capacity, file);
        if (line_length <= 0) {
            assert(feof(file));
            free(line);
            return NULL;
        } else if (line_length < 1) {
            // empty line
            free(line);
            continue;
        }

        assert(line[line_length - 1] == '\n');
        line[line_length - 1] = '\0';
        // Exclude any part of the line after a comment character
        char *comment_start = strchr(line, COMMENT_START);
        if (comment_start == NULL) return line;
        if (comment_start > line) {
            *comment_start = '\0';
            return line;
        }

        // If the entire line is a comment, skip it
        free(line);
    }
}

config_t *read_config_file(FILE *file) {
    size_t section_count = 0;
    linked_list_t *sections = init_linked_list();

    char *section_line = read_config_line(file);
    while (section_line != NULL) {
        assert(section_line[0] == SECTION_START);
        char *name_start = section_line + 1;
        char *name_end = strchr(name_start, SECTION_END);
        assert(name_end != NULL);
        assert(name_end[1] == '\0');
        section_count++;
        config_section_t *section = malloc(sizeof(*section));
        assert(section != NULL);
        size_t name_length = name_end - name_start;
        section->name = malloc(sizeof(char[name_length + 1]));
        assert(section->name != NULL);
        memcpy(section->name, name_start, sizeof(char[name_length]));
        section->name[name_length] = '\0';
        free(section_line);

        section->property_count = 0;
        linked_list_t *properties = init_linked_list();
        char *property_line;
        while (
            (property_line = read_config_line(file)) != NULL &&
            property_line[0] != SECTION_START
        ) {
            char *key_start = skip_whitespace(property_line);
            char *key_end = strstr(key_start, PROPERTY_SEPARATOR);
            assert(key_end != NULL);
            section->property_count++;
            config_property_t *property = malloc(sizeof(*property));
            assert(property != NULL);
            size_t key_length = key_end - key_start;
            property->key = malloc(sizeof(char[key_length + 1]));
            assert(property->key != NULL);
            memcpy(property->key, key_start, sizeof(char[key_length]));
            property->key[key_length] = '\0';
            property->value = strdup(key_end + strlen(PROPERTY_SEPARATOR));
            assert(property->value != NULL);
            list_push_back(properties, property);
            free(property_line);
        }

        section->properties = malloc(sizeof(config_property_t[section->property_count]));
        assert(section->properties != NULL);
        list_node_t *property = list_head(properties);
        for (size_t i = 0; i < section->property_count; i++) {
            section->properties[i] = *(config_property_t *) node_value(property);
            property = node_next(property);
        }
        free_linked_list(properties, free);
        list_push_back(sections, section);
        section_line = property_line;
    }
    fclose(file);

    config_t *config =
        malloc(sizeof(*config) + sizeof(config_section_t[section_count]));
    assert(config != NULL);
    config->section_count = section_count;
    list_node_t *section = list_head(sections);
    for (size_t i = 0; i < config->section_count; i++) {
        config->sections[i] = *(config_section_t *) node_value(section);
        section = node_next(section);
    }
    free_linked_list(sections, free);
    return config;
}

config_t *read_config(void) {
    return read_config_file(open_config_file("r"));
}

config_t *read_global_config(void) {
    char *home_directory = getenv("HOME");
    assert(home_directory != NULL);
    char config_path1[strlen(home_directory) + strlen(GLOBAL_CONFIG_PATH1) + 1];
    strcpy(config_path1, home_directory);
    strcat(config_path1, GLOBAL_CONFIG_PATH1);
    FILE *file = fopen(config_path1, "r");
    if (!file) {
        char config_path2[strlen(home_directory) + strlen(GLOBAL_CONFIG_PATH2) + 1];
        strcpy(config_path1, home_directory);
        strcat(config_path2, GLOBAL_CONFIG_PATH2);
        file = fopen(config_path2, "r");
        if (!file) {
            fprintf(stderr, "Failed to open global config file\n");
            exit(1);
        }
    }
    return read_config_file(file);
}

void write_config(const config_t *config) {
    FILE *file = open_config_file("w");
    for (size_t i = 0; i < config->section_count; i++) {
        const config_section_t *section = config->sections + i;
        int result = fputc(SECTION_START, file);
        assert(result != EOF);
        result = fputs(section->name, file);
        assert(result != EOF);
        result = fputc(SECTION_END, file);
        assert(result != EOF);
        result = fputc('\n', file);
        assert(result != EOF);
        for (size_t j = 0; j < section->property_count; j++) {
            config_property_t *property = section->properties + j;
            int result = fputc(PROPERTY_START, file);
            assert(result != EOF);
            result = fputs(property->key, file);
            assert(result != EOF);
            result = fputs(PROPERTY_SEPARATOR, file);
            assert(result != EOF);
            result = fputs(property->value, file);
            assert(result != EOF);
            result = fputc('\n', file);
            assert(result != EOF);
        }
    }
    fclose(file);
}

void free_config(config_t *config) {
    for (size_t i = 0; i < config->section_count; i++) {
        config_section_t *section = config->sections + i;
        free(section->name);
        for (size_t j = 0; j < section->property_count; j++) {
            config_property_t *property = section->properties + j;
            free(property->key);
            free(property->value);
        }
        free(section->properties);
    }
    free(config);
}

config_section_t *get_section(const config_t *config, const char *name) {
    for (size_t i = 0; i < config->section_count; i++) {
        config_section_t *section = (config_section_t *) config->sections + i;
        if (strcmp(section->name, name) == 0) return section;
    }
    return NULL;
}

char *get_branch_section_name(const char *branch_name) {
    char *section_name = malloc(sizeof(char[
        strlen(BRANCH_SECTION_PREFIX) +
        strlen(branch_name) +
        strlen(SECTION_SUFFIX) +
        1
    ]));
    assert(section_name != NULL);
    strcpy(section_name, BRANCH_SECTION_PREFIX);
    strcat(section_name, branch_name);
    strcat(section_name, SECTION_SUFFIX);
    return section_name;
}

config_section_t *get_branch_section(const config_t *config, const char *branch_name) {
    char *section_name = get_branch_section_name(branch_name);
    config_section_t *section = get_section(config, section_name);
    free(section_name);
    return section;
}

config_section_t *get_remote_section(const config_t *config, const char *remote_name) {
    char section_name[
        strlen(REMOTE_SECTION_PREFIX) +
        strlen(remote_name) +
        strlen(SECTION_SUFFIX) +
        1
    ];
    strcpy(section_name, REMOTE_SECTION_PREFIX);
    strcat(section_name, remote_name);
    strcat(section_name, SECTION_SUFFIX);
    return get_section(config, section_name);
}

bool is_branch_section(const config_section_t *section) {
    return starts_with(section->name, BRANCH_SECTION_PREFIX);
}

bool is_remote_section(const config_section_t *section) {
    return starts_with(section->name, REMOTE_SECTION_PREFIX);
}

char *copy_until_quote(char *start) {
    char *end = strstr(start, SECTION_SUFFIX);
    assert(end != NULL);
    size_t length = end - start;
    char *result = malloc(sizeof(char[length + 1]));
    assert(result != NULL);
    memcpy(result, start, sizeof(char[length]));
    result[length] = '\0';
    return result;
}

char *get_branch_name(const config_section_t *branch) {
    assert(is_branch_section(branch));
    return copy_until_quote(branch->name + strlen(BRANCH_SECTION_PREFIX));
}

char *get_remote_name(const config_section_t *remote) {
    assert(starts_with(remote->name, REMOTE_SECTION_PREFIX));
    return copy_until_quote(remote->name + strlen(REMOTE_SECTION_PREFIX));
}

char *get_property_value(const config_section_t *section, const char *key) {
    for (size_t i = 0; i < section->property_count; i++) {
        config_property_t *property = section->properties + i;
        if (strcmp(property->key, key) == 0) return property->value;
    }
    return NULL;
}

void set_property_value(
    const config_section_t *section,
    const char *key,
    const char *value
) {
    for (size_t i = 0; i < section->property_count; i++) {
        config_property_t *property = section->properties + i;
        if (strcmp(property->key, key) == 0) {
            free(property->value);
            property->value = (char *) value;
            return;
        }
    }
    fprintf(stderr, "No such property: %s\n", key);
    assert(false);
}
