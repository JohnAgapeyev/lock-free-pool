#ifndef QUEUE_H
#define QUEUE_H

#include "list.h"

typedef struct queue_node {
    void (*work)();
    void *args;
    struct list_head node;
} queue_node;

typedef struct LFQueue {
    struct queue_node *head;
    struct queue_node *tail;
    size_t size;
} LFQueue;

void push(LFQueue *q, void (*func)(), void *params);
queue_node *pop(LFQueue *q);

#endif
