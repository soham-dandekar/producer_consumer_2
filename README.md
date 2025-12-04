# Producer-Consumer Problem Implementation

## Overview
This program implements the classic Producer-Consumer synchronization problem using:
- System V Shared Memory (shmget, shmat, shmctl)
- System V Semaphores with atomic multi-semaphore operations (semop)
- Process creation using fork()

## Features
- 2 Producer processes that generate items and insert them into a shared buffer
- 1 Consumer process that removes and processes items from the buffer
- Circular buffer implemented in shared memory
- 3 semaphores for synchronization:
  - Mutex semaphore for mutual exclusion
  - Empty semaphore to count empty buffer slots
  - Full semaphore to count full buffer slots
- Atomic semaphore operations using semop with multiple operations

## Platform Support
- Linux: Fully supported
- macOS: Fully supported
- Windows: NOT supported (System V IPC is not available on Windows)

## Files
- `main.c`: Main program with process creation and semaphore operations
- `buff.h`: Header file defining the shared buffer structure
- `buff.c`: Buffer operations (insert, remove, display)
- `Makefile`: Build configuration

## Compilation

### Using Makefile (recommended):
```bash
make
```

### Manual compilation:
```bash
gcc -Wall -Wextra -O2 -c main.c
gcc -Wall -Wextra -O2 -c buff.c
gcc -Wall -Wextra -O2 -o producer_consumer main.o buff.o
```

## Running the Program
```bash
./producer_consumer
```

Or with make:
```bash
make run
```

## Cleanup
```bash
make clean
```

## How It Works

1. Parent process creates shared memory and semaphore set
2. Parent forks 3 child processes (2 producers, 1 consumer)
3. Each producer generates 10 items and inserts them into the buffer
4. Consumer removes and processes all 20 items
5. Synchronization ensures:
   - Producers wait when buffer is full
   - Consumer waits when buffer is empty
   - Mutual exclusion when accessing the buffer
6. Parent waits for all children and cleans up resources

## Output
The program displays:
- Process creation messages
- Producer activities (producing and inserting items)
- Consumer activities (waiting, removing, consuming items)
- Buffer state after each operation
- Cleanup messages

## Synchronization Logic

### Producer:
```
P(empty)  // Wait for empty slot
P(mutex)  // Acquire lock
<insert item>
V(mutex)  // Release lock
V(full)   // Signal full slot
```

### Consumer:
```
P(full)   // Wait for full slot
P(mutex)  // Acquire lock
<remove item>
V(mutex)  // Release lock
V(empty)  // Signal empty slot
```

Note: P() and V() operations on two semaphores are performed atomically using a single semop() call with multiple operations.

