CC = gcc
CFLAGS = -g -Wall -O0

.PHONY: all clean

all: hw1

hw1: main.o job.o
	$(CC) -o $@ $^ $(CFLAGS)

%.o: %.c
	$(CC) -o $@ -c $^ $(CFLAGS)

clean:
	rm -rf *.o
	rm hw1
