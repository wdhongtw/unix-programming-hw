CC = gcc
CFLAGS = -g -Wall -O0

.PHONY: all clean

all: hijack.so

%.o: %.c
	$(CC) -o $@ -c $^ $(CFLAGS)

%.so: %.c
	$(CC) -o $@ -shared -fPIC $^ $(CFLAGS) -ldl

clean:
	rm -rf *.o
	rm -f hijack.so index.*
