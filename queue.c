#define _POSIX_C_SOURCE 200809L //Enable POSIX 2008 features

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
        //Queue is empty
        q->head = newNode;
        init_list_head(&newNode->node);
    } else {
        assert(q->tail->node.next == NULL);
        list_insert_tail(&q->tail->node, &newNode->node);
    }
    q->tail = newNode;
    ++(q->size);
}

queue_node *queue_pop(LFQueue *q) {
    if (q->head == NULL) {
        return NULL; //Trying to pop an empty queue
    }
    queue_node *rtn = q->head;
    q->head = container_entry(rtn->node.next, struct queue_node, node);
    if (list_empty(&rtn->node)) {
        q->head = NULL;
        q->tail = NULL;
    } else {
        list_delete_head(&rtn->node);
    }
    --(q->size);
    return rtn;
}
