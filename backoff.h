#ifndef BACKOFF_H
#define BACKOFF_H

/**
 * Struct defining exponential backoff.
 * Each member represents the exponent of the backoff time in microseconds.
 * Ex. A current value of 8 will result in a backoff time of 2^8 microseconds.
 */
typedef struct {
    unsigned int curr; //Current backoff value
    unsigned int cap; //Maximum backoff value
} backoff_type;

void backoff(backoff_type *back);
void resetBackoff(backoff_type *back);

#endif
