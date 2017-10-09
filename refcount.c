#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "refcount.h"
#include "queue.h"

void init_refcount(struct refcount *ref) {
    ref->count = ATOMIC_VAR_INIT(1);
    if (pthread_spin_init(&(ref->spin), PTHREAD_PROCESS_PRIVATE) < 0) {
        perror("pthread_spin_init");
        exit(EXIT_FAILURE);
    }
}

void del_refcount(struct refcount *ref) {
    if (pthread_spin_destroy(&(ref->spin)) < 0) {
        perror("pthread_spin_destroy");
        exit(EXIT_FAILURE);
    }
}

queue_node *refcount_get(struct refcount *ref) {
    queue_node *rtn = NULL;
    pthread_spin_lock(&(ref->spin));
    if (!queue_empty(q)) {
        rtn = container_entry(ref, struct queue_node, refCounter);

        //Increment count atomically unless it is zero to begin with
        //If count is zero already and no increment is performed, return null
        size_t oldCount = atomic_load(&(ref->count));
        do {
            if (oldCount == 0) {
                break;
            }
        } while(atomic_compare_exchange_weak(&(ref->count), &oldCount, oldCount + 1));
        if (oldCount == 0) {
            rtn = NULL;
        }
    }
    pthread_spin_unlock(&(ref->spin));
    return rtn;
}

void queue_node_delete(struct refcount *ref) {
    queue_node *entry = container_entry(ref, struct queue_node, refCounter);
    del_refcount(ref);
    free(entry);
}

bool refcount_put(struct refcount *ref, void (*destructor)(struct refcount *)) {
    pthread_spin_lock(&(ref->spin));
    if (atomic_fetch_sub(&ref->count, 1) == 1) {
        destructor(ref);
        pthread_spin_unlock(&(ref->spin));
        return true;
    }
    pthread_spin_unlock(&(ref->spin));
    return false;
}

size_t refcount_read(struct refcount *ref) {
    return atomic_load(&(ref->count));
}
