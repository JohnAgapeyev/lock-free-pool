#ifndef LIST_H
#define LIST_H

#include <stdatomic.h>
#include <stddef.h>
#include <stdbool.h>
#include "refcount.h"

#define container_entry(ptr, type, member) \
    ((type *)((char *)(1 ? (ptr) : &((type *)0)->member) - offsetof(type, member)))

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos; pos = n, n = pos->next)

struct list_head {
    struct list_head * _Atomic next;
    struct list_head * _Atomic prev;
    struct refcount refCounter;
};

void init_list_head(struct list_head *head);

bool list_empty(const struct list_head *head);

//Insert node after the tail
void list_push_tail(struct list_head *head, struct list_head *new);

void list_delete_head(struct list_head *head);
void list_delete_tail(struct list_head *head);
void list_delete_node(struct list_head *prev, struct list_head *next);

void list_assign_pointer(struct list_head * _Atomic*head, struct list_head * _Atomic *new);
#endif
