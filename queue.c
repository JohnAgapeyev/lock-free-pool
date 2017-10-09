#include <stdatomic.h>
#include <assert.h>
#include <stdlib.h>
#include "list.h"
#include "queue.h"
#include "refcount.h"

void queue_init(LFQueue *q) {
    //q->head = ATOMIC_VAR_INIT(NULL);
    init_list_head(&(q->head));
    q->size = ATOMIC_VAR_INIT(0);
}

void queue_node_init(struct queue_node *qn) {
    qn->work = NULL;
    qn->args = NULL;
    init_list_head(&qn->node);
    init_refcount(&qn->refCounter);
}

bool queue_empty(const LFQueue *q) {
    return atomic_load(&q->size) == 0;
}

void queue_push(LFQueue *q, void (*func)(void *), void *params) {
    queue_node *newNode = malloc(sizeof(queue_node));
    if (newNode == NULL) {
        //Allocation failure
        abort();
    }

    queue_node_init(newNode);
    newNode->work = func;
    newNode->args = params;

    //Might need CAS loop instead of list call
    list_push_tail(&(q->head), &(newNode->node));

    atomic_fetch_add(&q->size, 1);
}

queue_node *queue_pop(LFQueue *q) {
    if (queue_empty(q)) {
        return NULL; //Trying to pop an empty queue
    }

    struct list_head *oldTail = q->head.prev;
    list_delete_tail(&(q->head));

    queue_node *rtn = container_entry(oldTail, queue_node, node);


    atomic_fetch_sub(&q->size, 1);

    return rtn;

#if 0
    queue_node *rtn = q->head;
    for (;;) {
        if (atomic_compare_exchange_weak(&q->head, &rtn,
                    container_entry(rtn->node.next, struct queue_node, node))) {
            break;
        }
        //Backoff
    }

    if (list_empty(&rtn->node)) {
        //Popped off the last entry
#if 0
            LFQueue *temp = malloc(sizeof(LFQueue));
            if (temp == NULL) {
                //perror("Allocation failure");
                abort();
            }
            temp->head = ATOMIC_VAR_INIT(NULL);
            temp->tail = ATOMIC_VAR_INIT(NULL);
            temp->size = ATOMIC_VAR_INIT(0);

            LFQueue *old = *q;
            *q = temp;
            free(old);
#else
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
#endif
        }
    } else {
        atomic_fetch_sub(&q->size, 1);
    }
done:
    return rtn;
#endif
}
