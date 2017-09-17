#ifndef HAZARD_H
#define HAZARD_H

#include <stdatomic.h>
#include "queue.h"

typedef struct {
    struct queue_node node;
    struct list_head head;
} retiredNode;

#define HAZARD_COUNT 2
#define THRESHOLD 1

struct HazardPointer {
    struct queue_node * hazard[HAZARD_COUNT];
    struct list_head recordList;
    atomic_bool active;
    retiredNode rList;
    atomic_size_t rCount;
};

typedef struct HazardPointer HazardPointer;

extern HazardPointer *  hpList;
extern atomic_size_t hpCount;

extern _Thread_local HazardPointer *myHP;

void init_hazard_pointer(HazardPointer *hp);
void scan(HazardPointer *hp);
void helpScan();
void allocateHazardPointer();
void retireHazardPointer(HazardPointer *hp);
void retireNode(struct queue_node * node);

#endif
