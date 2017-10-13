#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "refcount.h"
#include "queue.h"
#include "list.h"

pthread_mutex_t mut;
atomic_bool b = ATOMIC_VAR_INIT(false);

void init_refcount(struct refcount *ref, const size_t init) {
    if (!b) {
        atomic_store(&b, true);
        pthread_mutex_init(&mut, NULL);
    }
    ref->count = ATOMIC_VAR_INIT(init);
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

struct list_head *refcount_get(struct refcount *ref) {
    struct list_head *rtn = NULL;
    //pthread_spin_lock(&(ref->spin));
    pthread_mutex_lock(&mut);
    rtn = container_entry(ref, struct list_head, refCounter);

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
    //pthread_spin_unlock(&(ref->spin));
    pthread_mutex_unlock(&mut);
    return rtn;
}

void list_node_delete(struct refcount *ref) {
    struct list_head *node = container_entry(ref, struct list_head, refCounter);
    queue_node *entry = container_entry(node, struct queue_node, node);
    del_refcount(ref);
    free(entry);
}

bool refcount_put(struct refcount *ref, void (*destructor)(struct refcount *)) {
    pthread_mutex_lock(&mut);
    //pthread_spin_lock(&(ref->spin));
    if (atomic_fetch_sub(&ref->count, 1) == 1) {
        //pthread_spin_unlock(&(ref->spin));
        destructor(ref);
        pthread_mutex_unlock(&mut);
        return true;
    }
    //pthread_spin_unlock(&(ref->spin));
    pthread_mutex_unlock(&mut);
    return false;
}

size_t refcount_read(struct refcount *ref) {
    return atomic_load(&(ref->count));
}
