#ifndef LIST_H
#define LIST_H

#include <stddef.h>
#include <stdbool.h>

#define container_entry(ptr, type, member) \
    ((type *)((char *)(1 ? (ptr) : &((type *)0)->member) - offsetof(type, member)))

struct list_head {
    struct list_head *next;
    struct list_head *prev;
};

void init_list_head(struct list_head *head);

bool list_empty(struct list_head *head);

//Insert new node before list head
void list_insert_head(struct list_head *head, struct list_head *new);
//Insert new node after list tail
void list_insert_tail(struct list_head *tail, struct list_head *new);
//Insert new node before given node
void list_insert_node(struct list_head *node, struct list_head *new);

void list_delete_head(struct list_head *head);
void list_delete_tail(struct list_head *tail);
void list_delete_node(struct list_head *node);

void list_replace(struct list_head *old, struct list_head *new);

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos; pos = pos->next)

#define list_for_each_reverse(pos, head) \
    for (pos = (head)->prev; pos; pos = pos->prev)

#endif
