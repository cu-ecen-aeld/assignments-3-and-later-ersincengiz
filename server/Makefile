CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -Wall -Werror -g
LDFLAGS ?= -pthread

all: aesdsocket

clean:
	rm -f aesdsocket *.o

aesdsocket: aesdsocket.o
	$(CC) -o $@ $^ $(LDFLAGS)
