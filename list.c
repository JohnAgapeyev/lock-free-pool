#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include "list.h"

inline void init_list_head(struct list_head *head) {
    head->next = ATOMIC_VAR_INIT(head);
    head->prev = ATOMIC_VAR_INIT(head);
    //head->next = head;
    //head->prev = head;
}

inline bool list_empty(const struct list_head *head) {
    return head->next == head;
}

inline void list_push_tail(struct list_head *head, struct list_head *new) {
	new->next = head;
	new->prev = head->prev;
    head->prev->next = new;
    head->prev = new;
}

inline void list_delete_head(struct list_head *head) {
    list_delete_node(head->prev, head->next);
}

inline void list_delete_tail(struct list_head *head) {
    list_delete_node(head->prev->prev, head);
}

inline void list_delete_node(struct list_head *prev, struct list_head *next) {
    next->prev = prev;
    prev->next = next;
}

