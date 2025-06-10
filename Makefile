CC = gcc
CFLAGS = -Wall -g

all: user1 user2

user1_msgq: user1.c
	$(CC) $(CFLAGS) -o user1 user1.c

user2_msgq: user2_msgq.c
	$(CC) $(CFLAGS) -o user2 user2.c

clean:
	rm -f user1 user2