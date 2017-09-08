#define _POSIX_C_SOURCE 200809L //Enable POSIX 2008 features

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include "list.h"
#include "hazard.h"

HazardPointer *hpList;
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
    queue_node_init(&hp->nodeList.node);
    init_list_head(&hp->nodeList.listOfNodes);
    atomic_store(&hp->rCount, 0);
}

void scan(HazardPointer *hp) {
    struct queue_node *pointerList[hpCount];
    size_t pCount = 0;
    struct list_head *pos;
    //Add active hazard pointers to pList
    list_for_each(pos, &(hp->recordList)) {
        HazardPointer *entry = container_entry(pos, HazardPointer, recordList);
        for (int i = 0; i < HAZARD_COUNT; ++i) {
            if (entry->hazard[i]) {
                pointerList[pCount++] = entry->hazard[i];
                assert(pCount < hpCount);
            }
        }
    }
    qsort((void *) &pointerList, pCount, sizeof(struct queue_node *), pointerCompare);

    struct list_head scratch;
    struct list_head *pScratch = &scratch;
    //Compare pList and rList and remove any rList entries that are not in pList
    for (pos = myHP->nodeList.listOfNodes.next, pScratch = pos->next; 
            pos && pScratch; pos = pScratch, pScratch = pos->next) {
        retiredNode *listNode = container_entry(pos, retiredNode, listOfNodes);
        if (bsearch(&(listNode->node), pointerList, pCount, sizeof(struct queue_node *), pointerCompare) == NULL) {
            //Remove from linked list of retired nodes
            if (listNode->listOfNodes.prev == NULL) {
                if (!list_empty(&listNode->listOfNodes)) {
                    list_delete_head(&listNode->listOfNodes);
                }
            } else if (listNode->listOfNodes.next == NULL) {
                list_delete_tail(&listNode->listOfNodes);
            } else {
                list_delete_node(&listNode->listOfNodes);
            }
            init_list_head(&listNode->listOfNodes); //Null out the linked list portion
            atomic_fetch_sub(&hp->rCount, 1);
            //Handle re-use here instead of freeing the list entry
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
            struct list_head *temp = &entry->nodeList.listOfNodes;
            retiredNode *tempEntry = container_entry(temp, retiredNode, listOfNodes);
            list_delete_head(temp); 
            atomic_fetch_sub(&entry->rCount, 1);
            list_insert_head(&myHP->nodeList.listOfNodes, temp);
            myHP->nodeList = *tempEntry;
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

    list_insert_head(&hpList->recordList, &(newHP->recordList));

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

void retireNode(struct queue_node *node) {
    retiredNode *entry = malloc(sizeof(retiredNode));
    if (entry == NULL) {
        //Allocation failure
        abort();
    }
    entry->node = *node;
    init_list_head(&entry->listOfNodes);

    list_insert_head(&myHP->nodeList.listOfNodes, &entry->listOfNodes);

    myHP->nodeList = *entry;

    atomic_fetch_add(&myHP->rCount, 1);

    if (atomic_load(&myHP->rCount) > THRESHOLD) {
        scan(myHP);
        helpScan();
    }
}

