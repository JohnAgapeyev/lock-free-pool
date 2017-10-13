#include <stdatomic.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "list.h"
#include "queue.h"
#include "refcount.h"

void queue_init(LFQueue *q) {
    init_list_head(&(q->head));
    q->size = ATOMIC_VAR_INIT(0);
}

void queue_node_init(struct queue_node *qn) {
    qn->work = NULL;
    qn->args = NULL;
    init_list_head(&qn->node);
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

    struct list_head *temp = refcount_get(&(newNode->node.refCounter));
    assert(temp != NULL);
    assert(temp == &(newNode->node));

    //Might need CAS loop instead of list call
    //newNode->node.next = &(q->head);

    struct list_head * _Atomic tempHead = &(q->head);
    list_assign_pointer(&(newNode->node.next), &tempHead);

    //newNode->node.prev = q->head.prev;
    list_assign_pointer(&(newNode->node.prev), &(q->head.prev));
    //list_assign_pointer(&(newNode->node.prev), &(tempHead->prev));

    struct list_head * _Atomic tempNode = &(newNode->node);

    //q->head.prev->next = &(newNode->node);
    list_assign_pointer(&(q->head.prev->next), &tempNode);
    //list_assign_pointer(&(tempHead->prev->next), &tempNode);

    //q->head.prev = &(newNode->node);
    list_assign_pointer(&(q->head.prev), &tempNode);
    //list_assign_pointer(&(tempHead->prev), &tempNode);

    refcount_put(&(temp->refCounter), list_node_delete);

    atomic_fetch_add(&q->size, 1);
}

queue_node *queue_pop(LFQueue *q) {
    if (queue_empty(q)) {
        return NULL; //Trying to pop an empty queue
    }
    //struct list_head *oldHead = q->head.next;
    struct list_head *oldHead = refcount_get(&(q->head.next->refCounter));
    assert(oldHead != NULL);

    //q->head.next->next->prev = q->head.next->prev;
    list_assign_pointer(&(q->head.next->next->prev), &(q->head.next->prev));

    //q->head.next->prev->next = q->head.next->next;
    //list_assign_pointer(&(q->head.next->prev->next), &(q->head.next->next));
    list_assign_pointer(&(q->head.next), &(q->head.next->next));

    refcount_put(&(oldHead->next->refCounter), list_node_delete);
    refcount_put(&(oldHead->prev->refCounter), list_node_delete);

    queue_node *rtn = container_entry(oldHead, queue_node, node);

    atomic_fetch_sub(&q->size, 1);

    return rtn;
}
