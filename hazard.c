#include <stdio.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include "list.h"
#include "hazard.h"

HazardPointer * _Atomic hpList;
atomic_size_t hpCount;
_Thread_local HazardPointer *myHP;

int pointerCompare(const void *a, const void *b) {
    return (int)((struct queue_node *) a - (struct queue_node *) b) ;
}

void init_hazard_pointer(HazardPointer *hp) {
    for (int i = 0; i < HAZARD_COUNT; ++i) {
        hp->hazard[i] = ATOMIC_VAR_INIT(NULL);
    }
    init_list_head(&hp->recordList);
    hp->active = ATOMIC_VAR_INIT(false);

    init_list_head(&hp->rList);

    /*
    retiredNode *rTemp = malloc(sizeof(retiredNode));
    if (rTemp == NULL) {
        perror("Allocation failure");
        abort();
    }
    hp->rList = ATOMIC_VAR_INIT(rTemp);

    queue_node_init(&hp->rList->node);
    init_list_head(&hp->rList->head);
    */


    hp->rCount = ATOMIC_VAR_INIT(0);
}

void scan(HazardPointer *hp) {
    struct queue_node *pointerList[hpCount * HAZARD_COUNT];
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
    for (pos = &myHP->rList, pPosNext = pos->next;
            pPosNext && hp->rCount; pPosNext = pos->next) {

        /*
    for (pos = &myHP->rList->head, pPosNext = pos->next;
            pPosNext && hp->rCount; pPosNext = pos->next) {
        */

        retiredNode *listNode = container_entry(pPosNext, retiredNode, head);

        if (bsearch((const void *) &(listNode->node), pointerList, pCount,
                    sizeof(struct queue_node *), pointerCompare) == NULL) {
            //Remove from linked list of retired nodes
            //pos->next = pPosNext->next;

            struct list_head *oldPos = pos->next;
            for (;;) {
                if (atomic_compare_exchange_weak(&pos->next, &oldPos, pPosNext->next)) {
                    break;
                }
                //Backoff
            }

            //assert(hp->rCount > 0);
            atomic_fetch_sub(&hp->rCount, 1);
            //Handle re-use here instead of freeing the list entry
            //free(listNode);
            pPosNext = NULL;
        } else {
            //puts("Found one");
            pos = pPosNext;
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
            struct list_head *temp = &entry->rList;
            //retiredNode *tempEntry = container_entry(temp, retiredNode, head);
            list_delete_head(temp);
            atomic_fetch_sub(&entry->rCount, 1);

            /*
            temp->next = &myHP->rList->head;
            myHP->rList->head.next = temp->next;
            myHP->rList->node = tempEntry->node;
            */

            temp->next = &myHP->rList;
            myHP->rList.next = temp;

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
    /*
    entry->head.next = myHP->rList->head.next;
    myHP->rList->head.next = &entry->head;
    */
    entry->head.next = myHP->rList.next;
    myHP->rList.next = &entry->head;

    atomic_fetch_add(&myHP->rCount, 1);

    if (atomic_load(&myHP->rCount) > THRESHOLD) {
        scan(myHP);
        helpScan();
    }
}

void free_hazard_pointer(HazardPointer *hp) {
    struct list_head *pos;
    for (pos = &(hp->recordList); pos;) {
        HazardPointer *hp = container_entry(pos, HazardPointer, recordList);
        pos = pos->next;
        //free(hp->rList);

        /*
        struct list_head rListPos;
        for (rListPos = (&hp->rList); rListPos) {
            retiredNode *rNode = container_entry(rListPos, retired);
            rListPos = rListPos->next;
        }
        */




        free(hp);
    }
}
