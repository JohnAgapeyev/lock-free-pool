#define _POSIX_C_SOURCE 200809L //Enable POSIX 2008 features

#include <stdatomic.h>
#include <assert.h>
#include <stdlib.h>
#include "list.h"
#include "queue.h"

void queue_init(LFQueue *q) {
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
}

void queue_node_init(struct queue_node *qn) {
    qn->work = NULL;
    qn->args = NULL;
    init_list_head(&qn->node);
}

void queue_push(LFQueue *q, void (*func)(), void *params) {
    queue_node *newNode = malloc(sizeof(queue_node));
    if (newNode == NULL) {
        //Allocation failure
        abort();
    }

    newNode->work = func;
    newNode->args = params;

    if (q->head == NULL) {
        init_list_head(&newNode->node);
        queue_node *oldHead = q->head;
        for (;;) {
            //Queue is empty
            if (atomic_compare_exchange_weak(&q->head, &oldHead, newNode)) {
                break;
            }
            //Backoff
        }
    } else {
        //assert(q->tail->node.next == NULL);
        //list_insert_tail(&q->tail->node, &newNode->node);

        newNode->node.next = NULL;
        newNode->node.prev = &q->tail->node;

        struct list_head *oldNext = q->tail->node.next;
        for (;;) {
            if (atomic_compare_exchange_weak(&q->tail->node.next, &oldNext, &newNode->node)) {
                break;
            }
            //Backoff
        }
    }
    struct queue_node *oldTail = q->tail;
    for (;;) {
        if (atomic_compare_exchange_weak(&q->tail, &oldTail, newNode)) {
            break;
        }
    }
    atomic_fetch_add(&q->size, 1);
}

queue_node *queue_pop(LFQueue *q) {
    if (q->head == NULL) {
        return NULL; //Trying to pop an empty queue
    }

    queue_node *rtn = q->head;
    for (;;) {
        if (atomic_compare_exchange_weak(&q->head, &rtn, container_entry(rtn->node.next, struct queue_node, node))) {
            break;
        }
        //Backoff
    }

    if (list_empty(&rtn->node)) {
        //Popped off the last entry
        struct queue_node *oldHead = q->head;
        struct queue_node *oldTail = q->tail;
        for (;;) {
badhead:
            if (atomic_compare_exchange_weak(&q->head, &oldHead, NULL)) {
                for (;;) {
                    if (atomic_compare_exchange_weak(&q->tail, &oldTail, NULL)) {
                        if (q->head == NULL) {
                            atomic_store(&q->size, 0);
                            //Get out of here
                            goto done;
                        } else {
                            goto badhead;                      
                        }
                    }
                    //Backoff
                }
            }
            //Backoff
        }
    } else {
        //list_delete_head(&rtn->node);
        struct list_head *oldPrev = rtn->node.next->prev;
        for (;;) {
            if (atomic_compare_exchange_weak(&rtn->node.next->prev, &oldPrev, NULL)) {
                break;
            }
            //Backoff
        }
        atomic_fetch_sub(&q->size, 1);
    }
done:
    return rtn;
}
