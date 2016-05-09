CC = gcc
CFLAGS = -g -Wall -O0
TARGET = mysh

.PHONY: all clean

all: $(TARGET) showid

$(TARGET): $(TARGET).o
	$(CC) -o $@ $^ $(CFLAGS)

showid: showid.o
	$(CC) -o $@ $^ $(CFLAGS)

%.o: %.c
	$(CC) -o $@ -c $^ $(CFLAGS)

clean:
	rm -f *.o
	rm -f $(TARGET) showid
