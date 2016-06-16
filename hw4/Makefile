CC = gcc
CFLAGS = -g -Wall -O0
TARGET = myserver

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(TARGET).o
	$(CC) -o $@ $^ $(CFLAGS)

%.o: %.c
	$(CC) -o $@ -c $^ $(CFLAGS)

clean:
	rm -f *.o
	rm -f $(TARGET)
