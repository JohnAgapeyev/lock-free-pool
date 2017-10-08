#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include "list.h"

inline void init_list_head(struct list_head *head) {
    head->next = ATOMIC_VAR_INIT(NULL);
}

inline bool list_empty(struct list_head *head) {
    return head->next == NULL;
}

inline void list_push(struct list_head *tail, struct list_head *new) {
    new->next = NULL;
    tail->next = new;
}

inline void list_delete_head(struct list_head *head) {
    struct list_head *temp = head;
    if (list_empty(temp)) {
        head = NULL;
    } else {
        head = temp->next;
    }
}

inline void list_delete_tail(struct list_head *prev) {
    prev->next = NULL;
}

inline void list_delete_node(struct list_head *prev, struct list_head *node) {
    prev->next = node->next;
    node = NULL;
}

