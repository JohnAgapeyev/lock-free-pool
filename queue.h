#ifndef QUEUE_H
#define QUEUE_H

#include <stdatomic.h>
#include "list.h"

typedef struct queue_node {
    void (*work)();
    void *args;
    struct list_head node;
} queue_node;

typedef struct LFQueue {
    struct queue_node * _Atomic head;
    struct queue_node * _Atomic tail;
    atomic_size_t size;
} LFQueue;

void queue_init(LFQueue *q);
void queue_node_init(struct queue_node *qn);
void queue_push(LFQueue *q, void (*func)(), void *params);
queue_node *queue_pop(LFQueue * _Atomic q);

#endif
