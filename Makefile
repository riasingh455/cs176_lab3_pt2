CC = gcc
CFLAGS = -Wall -std=c99
TARGET = PingClient

all: $(TARGET)

$(TARGET): PingClient.c
	$(CC) $(CFLAGS) -o $(TARGET) PingClient.c

clean:
	rm -f $(TARGET)
