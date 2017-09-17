#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "queue.h"
#include "hazard.h"

#define MAX_THREADS_POWER 8
#define MAX_THREADS (1u << MAX_THREADS_POWER)

void print1() {
    fprintf(stderr, "1\n");
}

void print2() {
    fprintf(stderr, "2\n");
}

void print(void *num) {
    fprintf(stderr, "%d\n", *((int *) num));
}

void addQueue(LFQueue *q) {
    for (int i = 0 ; i < 20; ++i) {
#if 1
        if (rand() % 2 == 0) {
            queue_push(q, print1, NULL);
        } else {
            queue_push(q, print2, NULL);
        }
#else
        int *count = malloc(sizeof(int));
        *count = i;
        queue_push(q, print, count);
#endif
    }
}

void deleteQueue(LFQueue *q) {
    for (int i = 0 ; i < 20; ++i) {
        struct queue_node *job = queue_pop(q);
        if (job) {
            job->work(job->args);
            free(job);
        } else {
            fprintf(stderr, "Queue is empty\n");
        }
    }
}

void *threadTest1(void *q) {
    allocateHazardPointer();
    addQueue(q);
    retireHazardPointer(myHP);
    return NULL;
}

void *threadTest2(void *q) {
    allocateHazardPointer();
    deleteQueue(q);
    retireHazardPointer(myHP);
    return NULL;
}

void singleThreaded(LFQueue *q) {
    allocateHazardPointer();
    addQueue(q);
    deleteQueue(q);
    retireHazardPointer(myHP);
}

void multiThreaded(LFQueue *q) {
    pthread_t threads[MAX_THREADS];
    for (unsigned int i = 1; i <= MAX_THREADS_POWER; ++i) {
        printf("Testing with %d threads\n", (1u << i));
        for (unsigned int j = 0; j < (1u << i); ++j) {
            pthread_create(&threads[j], NULL, (rand() & 1) ? threadTest1 : threadTest2, q);
        }
        for (unsigned int j = 0; j < (1u << i); ++j) {
            pthread_join(threads[j], NULL);
        }
    }
}

int main(void) {
    LFQueue q;
    queue_init(&q);
    hpList = malloc(sizeof(HazardPointer));
    if (hpList == NULL) {
        //Allocation failure
        abort();
    }
    init_hazard_pointer(hpList);

    //Redirect stderr to /dev/null
    freopen("/dev/null", "w", stderr);

#if 0
    multiThreaded(&q);
#else
    singleThreaded(&q);
#endif

    free_hazard_pointer(hpList);
    return EXIT_SUCCESS;
}
