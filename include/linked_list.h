#ifndef LINKED_LIST_H
#define LINKED_LIST_H

typedef struct list_node list_node_t;
typedef struct linked_list linked_list_t;

typedef void (*free_func_t)(void *);

linked_list_t *init_linked_list(void);
void free_linked_list(linked_list_t *, free_func_t freer);

void *list_pop_front(linked_list_t *list);
void list_push_back(linked_list_t *, void *value);

typedef int (*compare_func_t)(void *, void *);
void list_sort(linked_list_t *, compare_func_t cmp);

// List traversal functions

list_node_t *list_head(const linked_list_t *);
list_node_t *node_next(const list_node_t *);
void *node_value(const list_node_t *);

#endif
