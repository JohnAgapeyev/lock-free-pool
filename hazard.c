#define _POSIX_C_SOURCE 200809L //Enable POSIX 2008 features

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include "list.h"
#include "hazard.h"

HazardPointer * hpList;
atomic_size_t hpCount;
_Thread_local HazardPointer *myHP;

int pointerCompare(const void *a, const void *b) {
    return (int)((struct queue_node *) a - (struct queue_node *) b) ;
}

void init_hazard_pointer(HazardPointer *hp) {
    for (int i = 0; i < HAZARD_COUNT; ++i) {
        hp->hazard[i] = NULL;
    }
    init_list_head(&hp->recordList);
    atomic_store(&hp->active, false);
    queue_node_init(&hp->rList.node);
    init_list_head(&hp->rList.head);
    atomic_store(&hp->rCount, 0);
}

void scan(HazardPointer *hp) {
    //THIS IS A BIG PROBLEM, CHANGE ARRAY SIZE TO PREVENT OVERFLOW
    struct queue_node *pointerList[hp->rCount];
    size_t pCount = 0;
    struct list_head *pos;
    //Add active hazard pointers to pList
    list_for_each(pos, &(hp->recordList)) {
        HazardPointer *entry = container_entry(pos, HazardPointer, recordList);
        for (int i = 0; i < HAZARD_COUNT; ++i) {
            if (entry->hazard[i]) {
                pointerList[pCount++] = entry->hazard[i];
            }
        }
    }
    //If pCount == 0 then pList will contain garbage, leading to awkward subtraction issues
    qsort((void *) &pointerList, pCount, sizeof(struct queue_node *), pointerCompare);

    struct list_head posNext;
    struct list_head *pPosNext = &posNext;
    //Compare pList and rList and remove any rList entries that are not in pList
    for (pos = &myHP->rList.head, pPosNext = pos->next; 
            pPosNext && hp->rCount; pos = pPosNext, pPosNext = pos->next) {

        retiredNode *listNode = container_entry(pPosNext, retiredNode, head);

        if (bsearch((const void *) &(listNode->node), pointerList, pCount, 
                    sizeof(struct queue_node *), pointerCompare) == NULL) {
            if (hp->rCount == 0) {
                //Don't try and remove an empty entry
                break;
            }

            //Remove from linked list of retired nodes
            if (listNode->head.next != NULL) {
                pos->next = listNode->head.next;
            }

            assert(hp->rCount > 0);
            atomic_fetch_sub(&hp->rCount, 1);
            //Handle re-use here instead of freeing the list entry
        } else {
            //puts("Found one");
        }
    }
}

void helpScan() {
    struct list_head *pos;
    list_for_each(pos, &hpList->recordList) {
        HazardPointer *entry = container_entry(pos, HazardPointer, recordList);
        if (atomic_load(&entry->active)) {
            continue;
        }
        if (atomic_exchange(&entry->active, true)) {
            continue;
        }
        while (entry->rCount > 0) {
            struct list_head *temp = &entry->rList.head;
            retiredNode *tempEntry = container_entry(temp, retiredNode, head);
            list_delete_head(temp); 
            atomic_fetch_sub(&entry->rCount, 1);

            temp->next = &myHP->rList.head;
            myHP->rList.head = *temp;

            myHP->rList = *tempEntry;
            atomic_fetch_add(&myHP->rCount, 1);
            if (atomic_load(&myHP->rCount) > THRESHOLD) {
                scan(hpList);
            }
        }
        atomic_store(&entry->active, false);
    }
}

void allocateHazardPointer() {
    struct list_head *pos;
    list_for_each(pos, &(hpList->recordList)) {
        HazardPointer *hp = container_entry(pos, HazardPointer, recordList);
        if (atomic_load(&(hp->active))) {
            //Flag is already active, ignore it
            continue;
        }
        if (atomic_exchange(&(hp->active), true)) {
            //Flag is already active, ignore it
            continue;
        }
        myHP = hp;
        return;
    }

    atomic_fetch_add(&hpCount, 1);

    HazardPointer *newHP = malloc(sizeof(HazardPointer));
    if (newHP == NULL) {
        //Allocation failure
        abort();
    }

    init_hazard_pointer(newHP);

    newHP->recordList.next = &hpList->recordList;
    //hpList->recordList.next = &newHP->recordList;

    HazardPointer *oldHead = hpList;
    for (;;) {
        if (atomic_compare_exchange_weak(&hpList, &oldHead, newHP)) {
            break;
        }
        //Backoff
    }
    myHP = newHP;
}

void retireHazardPointer(HazardPointer *hp) {
    for (int i = 0 ; i < HAZARD_COUNT; ++i) {
        hp->hazard[i] = NULL;
    }
    atomic_store(&hp->active, false);
}

void retireNode(struct queue_node * node) {
    retiredNode *entry = malloc(sizeof(retiredNode));
    if (entry == NULL) {
        //Allocation failure
        abort();
    }
    entry->node = *node;
    entry->head.next = NULL;

    //Add entry after head of rList
    entry->head.next = myHP->rList.head.next;
    myHP->rList.head.next = &entry->head;

    atomic_fetch_add(&myHP->rCount, 1);

    if (atomic_load(&myHP->rCount) > THRESHOLD) {
        scan(myHP);
        helpScan();
    }
}

