#include "buff.h"
#include <string.h>

/* Initialize the shared buffer to empty state */
void buffer_init(shared_buffer_t *buf) {
    buf->head = 0;
    buf->tail = 0;
    buf->count = 0;
    
    /* Clear the buffer array */
    memset(buf->buffer, 0, sizeof(buf->buffer));
}

/* Insert item into the circular buffer */
void buffer_insert(shared_buffer_t *buf, int item) {
    /* Insert item at head position */
    buf->buffer[buf->head] = item;
    
    /* Move head to next position (circular) */
    buf->head = (buf->head + 1) % BUFFER_SIZE;
    
    /* Increment count of items in buffer */
    buf->count++;
}

/* Remove and return item from the circular buffer */
int buffer_remove(shared_buffer_t *buf) {
    /* Get item from tail position */
    int item = buf->buffer[buf->tail];
    
    /* Clear the slot */
    buf->buffer[buf->tail] = 0;
    
    /* Move tail to next position (circular) */
    buf->tail = (buf->tail + 1) % BUFFER_SIZE;
    
    /* Decrement count of items in buffer */
    buf->count--;
    
    return item;
}

/* Display the current state of the buffer for debugging */
void buffer_display(shared_buffer_t *buf, const char *process_name) {
    printf("[%s] Buffer: [", process_name);
    
    for (int i = 0; i < BUFFER_SIZE; i++) {
        if (i > 0) printf(", ");
        printf("%d", buf->buffer[i]);
    }
    
    printf("] (count=%d, head=%d, tail=%d)\n", 
           buf->count, buf->head, buf->tail);
    fflush(stdout);
}

