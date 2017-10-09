#ifndef QUEUE_H
#define QUEUE_H

#include <stdatomic.h>
#include "list.h"

typedef struct queue_node {
    void (*work)(void *);
    void *args;
    struct list_head node;
} queue_node;

typedef struct LFQueue {
    struct list_head head;
    atomic_size_t size;
} LFQueue;

void queue_init(LFQueue *q);
void queue_node_init(queue_node *qn);
void queue_push(LFQueue *q, void (*func)(void *), void *params);
queue_node *queue_pop(LFQueue *q);
bool queue_empty(const LFQueue *q);

#endif
