#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>
#include "buff.h"

/* Semaphore indices in the semaphore set */
#define SEM_MUTEX 0   /* Semaphore for mutual exclusion (binary semaphore) */
#define SEM_EMPTY 1   /* Semaphore to count empty slots */
#define SEM_FULL 2    /* Semaphore to count full slots */

/* Number of items each producer will produce */
#define ITEMS_PER_PRODUCER 10

/* Union for semctl operations - required on some systems */
/* On macOS, semun is already defined in sys/sem.h */
#if defined(__linux__)
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};
#endif

/* Logging function prototypes */
void log_info(const char *format, ...);
void log_error(const char *message);
void log_producer(int producer_id, const char *format, ...);
void log_consumer(const char *format, ...);

/* Function prototypes */
void producer_process(int producer_id, shared_buffer_t *buf, int sem_id);
void consumer_process(shared_buffer_t *buf, int sem_id);
void atomic_wait_two(int sem_id, int sem1, int sem2);
void atomic_signal_two(int sem_id, int sem1, int sem2);

int main() {
    int shm_id, sem_id;
    shared_buffer_t *shared_buf;
    pid_t pid1, pid2, pid3;
    key_t shm_key, sem_key;
    union semun sem_union;
    unsigned short sem_values[3];
    
    log_info("=== Producer-Consumer Problem with Shared Memory and Semaphores ===\n\n");
    
    /* Generate unique keys for shared memory and semaphore set */
    shm_key = ftok(".", 'S');
    sem_key = ftok(".", 'E');
    
    if (shm_key == -1 || sem_key == -1) {
        log_error("ftok failed");
        exit(1);
    }
    
    /* Create shared memory segment for the buffer */
    shm_id = shmget(shm_key, sizeof(shared_buffer_t), IPC_CREAT | 0666);
    if (shm_id == -1) {
        log_error("shmget failed");
        exit(1);
    }
    log_info("Shared memory created (ID: %d)\n", shm_id);
    
    /* Attach shared memory to process address space */
    shared_buf = (shared_buffer_t *)shmat(shm_id, NULL, 0);
    if (shared_buf == (void *)-1) {
        log_error("shmat failed");
        exit(1);
    }
    log_info("Shared memory attached\n");
    
    /* Initialize the shared buffer */
    buffer_init(shared_buf);
    log_info("Buffer initialized (size: %d)\n\n", BUFFER_SIZE);
    
    /* Create semaphore set with 3 semaphores */
    sem_id = semget(sem_key, 3, IPC_CREAT | 0666);
    if (sem_id == -1) {
        log_error("semget failed");
        exit(1);
    }
    log_info("Semaphore set created (ID: %d)\n", sem_id);
    
    /* Initialize semaphore values */
    sem_values[SEM_MUTEX] = 1;            /* Mutex starts at 1 (unlocked) */
    sem_values[SEM_EMPTY] = BUFFER_SIZE;  /* Initially all slots are empty */
    sem_values[SEM_FULL] = 0;             /* Initially no slots are full */
    
    sem_union.array = sem_values;
    if (semctl(sem_id, 0, SETALL, sem_union) == -1) {
        log_error("semctl SETALL failed");
        exit(1);
    }
    log_info("Semaphores initialized: mutex=1, empty=%d, full=0\n\n", BUFFER_SIZE);
    
    /* Create first producer process */
    pid1 = fork();
    if (pid1 == -1) {
        log_error("fork failed for producer 1");
        exit(1);
    }
    
    if (pid1 == 0) {
        /* Child process: Producer 1 */
        producer_process(1, shared_buf, sem_id);
        exit(0);
    }
    
    /* Create second producer process */
    pid2 = fork();
    if (pid2 == -1) {
        log_error("fork failed for producer 2");
        exit(1);
    }
    
    if (pid2 == 0) {
        /* Child process: Producer 2 */
        producer_process(2, shared_buf, sem_id);
        exit(0);
    }
    
    /* Create consumer process */
    pid3 = fork();
    if (pid3 == -1) {
        log_error("fork failed for consumer");
        exit(1);
    }
    
    if (pid3 == 0) {
        /* Child process: Consumer */
        consumer_process(shared_buf, sem_id);
        exit(0);
    }
    
    /* Parent process: wait for all children to complete */
    log_info("Parent: Created Producer1 (PID=%d), Producer2 (PID=%d), Consumer (PID=%d)\n\n",
           pid1, pid2, pid3);
    
    /* Wait for all child processes */
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    waitpid(pid3, NULL, 0);
    
    log_info("\n=== All processes completed ===\n\n");
    
    /* Detach shared memory */
    if (shmdt(shared_buf) == -1) {
        log_error("shmdt failed");
    }
    
    /* Remove shared memory segment */
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        log_error("shmctl IPC_RMID failed");
    } else {
        log_info("Shared memory removed\n");
    }
    
    /* Remove semaphore set */
    if (semctl(sem_id, 0, IPC_RMID) == -1) {
        log_error("semctl IPC_RMID failed");
    } else {
        log_info("Semaphore set removed\n");
    }
    
    log_info("\nProgram terminated successfully\n");
    return 0;
}

/* Logging function implementations */

/* Log informational messages with variable arguments */
void log_info(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    fflush(stdout);
}

/* Log error messages with errno information */
void log_error(const char *message) {
    perror(message);
    fflush(stderr);
}

/* Log producer-specific messages with variable arguments */
void log_producer(int producer_id, const char *format, ...) {
    va_list args;
    printf("Producer%d: ", producer_id);
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    fflush(stdout);
}

/* Log consumer-specific messages with variable arguments */
void log_consumer(const char *format, ...) {
    va_list args;
    printf("Consumer: ");
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    fflush(stdout);
}

/* Producer process implementation */
void producer_process(int producer_id, shared_buffer_t *buf, int sem_id) {
    int item;
    
    /* Seed random number generator with process-specific seed */
    srand(time(NULL) + getpid());
    
    for (int i = 0; i < ITEMS_PER_PRODUCER; i++) {
        /* Generate item to produce (random number between 100-999) */
        item = (rand() % 900) + 100;
        
        log_producer(producer_id, "Producing item %d\n", item);
        
        /* P(empty) and P(mutex) - atomic operation */
        /* Wait for empty slot and acquire mutex */
        atomic_wait_two(sem_id, SEM_EMPTY, SEM_MUTEX);
        
        /* Critical section: insert item into buffer */
        buffer_insert(buf, item);
        log_producer(producer_id, "Inserted item %d\n", item);
        buffer_display(buf, "Producer");
        
        /* V(mutex) and V(full) - atomic operation */
        /* Release mutex and signal full slot */
        atomic_signal_two(sem_id, SEM_MUTEX, SEM_FULL);
        
        /* Sleep for a short time to demonstrate synchronization */
        usleep((rand() % 500000) + 100000);  /* 100-600ms */
    }
    
    log_producer(producer_id, "Finished producing %d items\n", ITEMS_PER_PRODUCER);
}

/* Consumer process implementation */
void consumer_process(shared_buffer_t *buf, int sem_id) {
    int item;
    int total_items = 2 * ITEMS_PER_PRODUCER;  /* Total items from both producers */
    
    /* Seed random number generator */
    srand(time(NULL) + getpid());
    
    for (int i = 0; i < total_items; i++) {
        log_consumer("Waiting to consume...\n");
        
        /* P(full) and P(mutex) - atomic operation */
        /* Wait for full slot and acquire mutex */
        atomic_wait_two(sem_id, SEM_FULL, SEM_MUTEX);
        
        /* Critical section: remove item from buffer */
        item = buffer_remove(buf);
        log_consumer("Removed item %d\n", item);
        buffer_display(buf, "Consumer");
        
        /* V(mutex) and V(empty) - atomic operation */
        /* Release mutex and signal empty slot */
        atomic_signal_two(sem_id, SEM_MUTEX, SEM_EMPTY);
        
        log_consumer("Consumed item %d\n", item);
        
        /* Sleep for a short time to demonstrate synchronization */
        usleep((rand() % 600000) + 200000);  /* 200-800ms */
    }
    
    log_consumer("Finished consuming %d items\n", total_items);
}

/* Atomic wait operation on two semaphores */
void atomic_wait_two(int sem_id, int sem1, int sem2) {
    struct sembuf operations[2];
    
    /* First semaphore: P operation (wait, decrement) */
    operations[0].sem_num = sem1;
    operations[0].sem_op = -1;   /* Decrement by 1 */
    operations[0].sem_flg = 0;   /* Wait if not available */
    
    /* Second semaphore: P operation (wait, decrement) */
    operations[1].sem_num = sem2;
    operations[1].sem_op = -1;   /* Decrement by 1 */
    operations[1].sem_flg = 0;   /* Wait if not available */
    
    /* Execute both operations atomically */
    if (semop(sem_id, operations, 2) == -1) {
        log_error("semop wait failed");
        exit(1);
    }
}

/* Atomic signal operation on two semaphores */
void atomic_signal_two(int sem_id, int sem1, int sem2) {
    struct sembuf operations[2];
    
    /* First semaphore: V operation (signal, increment) */
    operations[0].sem_num = sem1;
    operations[0].sem_op = 1;    /* Increment by 1 */
    operations[0].sem_flg = 0;
    
    /* Second semaphore: V operation (signal, increment) */
    operations[1].sem_num = sem2;
    operations[1].sem_op = 1;    /* Increment by 1 */
    operations[1].sem_flg = 0;
    
    /* Execute both operations atomically */
    if (semop(sem_id, operations, 2) == -1) {
        log_error("semop signal failed");
        exit(1);
    }
}

