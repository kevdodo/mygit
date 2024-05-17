// Simple linked list implementation; only includes operations that I need
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "linked_list.h"

struct list_node {
   list_node_t *next;
   void *value;
};
struct linked_list {
    list_node_t *head;
    list_node_t **tail;
};

linked_list_t *init_linked_list() {
    linked_list_t *list = malloc(sizeof(*list));
    assert(list != NULL);
    list->head = NULL;
    list->tail = &list->head;
    return list;
}

void free_linked_list(linked_list_t *list, free_func_t freer) {
    for (list_node_t *node = list_head(list), *next; node != NULL; node = next) {
        if (freer != NULL) freer(node_value(node));
        next = node_next(node);
        free(node);
    }
    free(list);
}

void free_linked_list_char(linked_list_t *list) {
    for (list_node_t *node = list_head(list), *next; node != NULL; node = next) {
        // if (freer != NULL) freer(node_value(node));
        // node value here should be a char *

        free(node_value(node));

        next = node_next(node);
        free(node);
    }
    free(list);
}

void *list_pop_front(linked_list_t *list) {
    list_node_t *head = list->head;
    if (head == NULL) return NULL;

    void *value = head->value;
    list->head = head->next;
    if (list->tail == &head->next) list->tail = &list->head;
    free(head);
    return value;
}

void list_push_back(linked_list_t *list, void *value) {
    list_node_t *node = malloc(sizeof(*node));
    assert(node != NULL);
    node->value = value;
    node->next = NULL;

    *list->tail = node;
    list->tail = &node->next;
}

linked_list_t split(list_node_t *list) {
    linked_list_t other_list = {.tail = &other_list.head};
    while (list != NULL) {
        // Add the next element to the other list, if it exists.
        // If there are an odd number of nodes, `list` will have 1 more than `other_list`.
        list_node_t *other_next = list->next;
        if (other_next == NULL) break;

        // Remove `other_next` from `list`
        list->next = other_next->next;
        list = list->next;
        // Append `other_next` to `other_list`
        *other_list.tail = other_next;
        other_list.tail = &other_next->next;
    }
    *other_list.tail = NULL;
    return other_list;
}

void list_sort(linked_list_t *list, compare_func_t cmp) {
    linked_list_t other_list = split(list->head);
    if (other_list.head == NULL) return; // 1-element list needs no sorting

    // Recursively sort each half of the list
    list_sort(list, cmp);
    list_sort(&other_list, cmp);

    // Merge `list` and `other_list` into `list`
    list_node_t *list_next = list->head;
    list_node_t *other_next = other_list.head;
    list->tail = &list->head;
    while (true) {
        // Decide which element to append to the list.
        // If either sublist is empty, add from the other. If both are empty, we are done.
        // Otherwise, compare the values and take the smaller one.
        list_node_t **next;
        if (list_next == NULL) {
            if (other_next == NULL) break;

            next = &other_next;
        }
        else {
            next = other_next == NULL
                ? &list_next
                : cmp(list_next->value, other_next->value) < 0 ? &list_next : &other_next;
        }

        // Append the element to the list and remove it from its sublist
        *list->tail = *next;
        list->tail = &(*next)->next;
        *next = (*next)->next;
    }
    *list->tail = NULL;
}

list_node_t *list_head(const linked_list_t *list) {
    if (list == NULL) return NULL;
    return list->head;
}
list_node_t *node_next(const list_node_t *node) {
    return node->next;
}
void *node_value(const list_node_t *node) {
    return node->value;
}
