#ifndef REFCOUNT_H
#define REFCOUNT_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>

typedef struct queue_node queue_node;

struct refcount {
    atomic_size_t count;
    pthread_spinlock_t spin;
};

void init_refcount(struct refcount *ref);
void del_refcount(struct refcount *ref);
queue_node *refcount_get(struct refcount *ref);
bool refcount_put(struct refcount *ref, void (*destructor)(struct refcount *));
size_t refcount_read(struct refcount *ref);
void queue_node_delete(struct refcount *ref);

#endif
