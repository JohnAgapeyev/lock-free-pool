#ifndef LIST_H
#define LIST_H

#include <stddef.h>
#include <stdbool.h>

#define container_entry(ptr, type, member) \
    ((type *)((char *)(1 ? (ptr) : &((type *)0)->member) - offsetof(type, member)))

struct list_head {
    struct list_head *next;
};

void init_list_head(struct list_head *head);

bool list_empty(struct list_head *head);

//Insert node after the tail
void list_push(struct list_head *tail, struct list_head *new);

void list_delete_head(struct list_head *head);
void list_delete_tail(struct list_head *prev);
void list_delete_node(struct list_head *prev, struct list_head *node);

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos; pos = pos->next)

/*
#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos; pos = n, n = pos->next)
*/

#endif
