# Makefile for Producer-Consumer problem

CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = producer_consumer
OBJS = main.o buff.o

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compile main.c
main.o: main.c buff.h
	$(CC) $(CFLAGS) -c main.c

# Compile buff.c
buff.o: buff.c buff.h
	$(CC) $(CFLAGS) -c buff.c

# Clean build artifacts
clean:
	rm -f $(TARGET) $(OBJS)

# Run the program
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run

