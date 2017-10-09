#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "queue.h"
#include "refcount.h"

#define MAX_THREADS_POWER 10
#define MAX_THREADS (1u << MAX_THREADS_POWER)
#define TASK_COUNT 10000

void print1() {
#if 1
    fprintf(stderr, "1\n");
#else
    fprintf(stdout, "1\n");
#endif
}

void print2() {
#if 1
    fprintf(stderr, "2\n");
#else
    fprintf(stdout, "2\n");
#endif
}

void print(void *num) {
#if 1
    fprintf(stderr, "%d\n", *((int *) num));
#else
    fprintf(stdout, "%d\n", *((int *) num));
#endif
}

void addQueue(LFQueue *q) {
    for (int i = 0 ; i < TASK_COUNT; ++i) {
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
    for (int i = 0 ; i < TASK_COUNT; ++i) {
        struct queue_node *job = queue_pop(q);
        if (job) {
            job->work(job->args);
            //printf("Job ref count: %zu\n", refcount_read(&(job->node.refCounter)));
            refcount_put(&(job->node.refCounter), list_node_delete);
        } else {
#if 1
    fprintf(stderr, "Queue is empty\n");
#else
    fprintf(stdout, "Queue is empty\n");
#endif
        }
    }
}

void *threadTest1(void *q) {
    addQueue(q);
    return NULL;
}

void *threadTest2(void *q) {
    deleteQueue(q);
    return NULL;
}

void singleThreaded(LFQueue *q) {
    addQueue(q);
    deleteQueue(q);
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
    //LFQueue q;
    LFQueue *q = malloc(sizeof(LFQueue));
    if (q == NULL) {
        perror("Allocation failure");
        abort();
    }
    queue_init(q);

    //Redirect stderr to /dev/null
#if 1
    freopen("/dev/null", "w", stderr);
#else
    freopen("/dev/null", "w", stdout);
#endif

#if 1
    multiThreaded(q);
#else
    singleThreaded(q);
#endif

    free(q);
    return EXIT_SUCCESS;
}
