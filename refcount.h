#ifndef REFCOUNT_H
#define REFCOUNT_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>

struct list_head;

struct refcount {
    atomic_size_t count;
    pthread_spinlock_t spin;
};

void init_refcount(struct refcount *ref, const size_t init);
void del_refcount(struct refcount *ref);
struct list_head *refcount_get(struct refcount *ref);
bool refcount_put(struct refcount *ref, void (*destructor)(struct refcount *));
size_t refcount_read(struct refcount *ref);
void list_node_delete(struct refcount *ref);

#endif
