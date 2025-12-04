#ifndef BUFF_H
#define BUFF_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

/* Buffer size - number of slots in circular buffer */
#define BUFFER_SIZE 5

/* Shared buffer structure for producer-consumer problem */
typedef struct {
    int buffer[BUFFER_SIZE];  /* Circular buffer array */
    int head;                 /* Index where producer inserts next item */
    int tail;                 /* Index where consumer removes next item */
    int count;                /* Current number of items in buffer */
} shared_buffer_t;

/* Function prototypes for buffer operations */

/* Initialize the shared buffer */
void buffer_init(shared_buffer_t *buf);

/* Insert item into buffer (used by producer) */
void buffer_insert(shared_buffer_t *buf, int item);

/* Remove item from buffer (used by consumer) */
int buffer_remove(shared_buffer_t *buf);

/* Display current buffer state */
void buffer_display(shared_buffer_t *buf, const char *process_name);

#endif /* BUFF_H */

