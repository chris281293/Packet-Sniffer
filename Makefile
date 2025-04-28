CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lpcap -lncurses -lpthread

# Source files
SRCS = main.c packet_listener.c ringbuffer.c
OBJS = $(SRCS:.c=.o)

TARGET = sniffer

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
