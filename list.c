#define _POSIX_C_SOURCE 200809L //Enable POSIX 2008 features

#include <stdbool.h>
#include <stdlib.h>
#include "list.h"

inline void init_list_head(struct list_head *head) {
    head->next = NULL;
    head->prev = NULL;
}

inline bool list_empty(struct list_head *head) {
    return head->next == NULL;
}

inline void list_insert_head(struct list_head *head, struct list_head *new) {
    new->prev = NULL;
    new->next = head;
    head->prev = new;
}

inline void list_insert_tail(struct list_head *tail, struct list_head *new) {
    new->next = NULL;
    new->prev = tail;
    tail->next = new;
}

inline void list_insert_node(struct list_head *node, struct list_head *new) {
    new->prev = node->prev;
    node->prev->next = new;
    new->next = node;
    node->prev = new;
}

inline void list_delete_head(struct list_head *head) {
    head->next->prev = NULL;
}

inline void list_delete_tail(struct list_head *tail) {
    tail->prev->next = NULL;
}

inline void list_delete_node(struct list_head *node) {
    node->prev->next = node->next; 
    node->next->prev = node->prev;
}

inline void list_replace(struct list_head *old, struct list_head *new) {
    new->next = old->next;
    new->next->prev = new;
    new->prev = old->prev;
    new->prev->next = new;
}

