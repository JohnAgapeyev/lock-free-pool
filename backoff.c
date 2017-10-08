#include <time.h>
#include <math.h>
#include <assert.h>
#include "backoff.h"

void backoff(backoff_type *back) {
    assert(back != NULL);
    assert(back->curr <= back->cap);
    assert(back->curr < 32); //Prevent over-shifting

    struct timespec ts = {0, (1ul << back->curr) * 1000};
    nanosleep(&ts, NULL);
    ++(back->curr);
    if (back->curr > back->cap) {
        back->curr = back->cap;
    }
}

void resetBackoff(backoff_type *back) {
    assert(back != NULL);
    back->curr = 1;
}
