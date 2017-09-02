#define _POSIX_C_SOURCE 200809L //Enable POSIX 2008 features

#include <assert.h>
#include <stdlib.h>
#include "list.h"
#include "queue.h"

void push(LFQueue *q, void (*func)(), void *params) {
    assert(q->tail->node.next == NULL);

    queue_node *newNode = malloc(sizeof(queue_node));
    if (newNode == NULL) {
        //Allocation failure
        //abort();
    }

    newNode->work = func;
    newNode->args = params;

    list_insert_tail(&q->tail->node, &newNode->node);

    q->tail = newNode;
    ++(q->size);
}

queue_node *pop(LFQueue *q) {
    queue_node *rtn = q->head;
    q->head = container_entry(rtn->node.next, struct queue_node, node);
    list_delete_head(&rtn->node);
    --(q->size);
    return rtn;
}
