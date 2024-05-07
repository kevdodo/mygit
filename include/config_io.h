#ifndef CONFIG_IO_H
#define CONFIG_IO_H

#include <stdbool.h>
#include <stddef.h>

extern const char MERGE_KEY[];
extern const char REMOTE_KEY[];
extern const char URL_KEY[];

typedef struct {
    char *key;
    char *value;
} config_property_t;

typedef struct {
    char *name;
    size_t property_count;
    config_property_t *properties;
} config_section_t;

typedef struct {
    size_t section_count;
    config_section_t sections[];
} config_t;

config_t *read_config(void);
config_t *read_global_config(void);
void write_config(const config_t *);
void free_config(config_t *);

config_section_t *get_section(const config_t *, const char *name);
char *get_branch_section_name(const char *branch_name);
config_section_t *get_branch_section(const config_t *, const char *branch_name);
config_section_t *get_remote_section(const config_t *, const char *remote_name);
bool is_branch_section(const config_section_t *);
bool is_remote_section(const config_section_t *);
char *get_branch_name(const config_section_t *);
char *get_remote_name(const config_section_t *);
char *get_property_value(const config_section_t *, const char *key);
void set_property_value(const config_section_t *, const char *key, const char *value);

#endif // #ifndef CONFIG_IO_H
