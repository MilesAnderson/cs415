CC = gcc
CFLAGS = -g -Wall -pthread

OBJS = park.o thread_queue.o

all: park

park: $(OBJS)
	$(CC) $(CFLAGS) -o park $(OBJS)

park.o: park.c thread_queue.h
	$(CC) $(CFLAGS) -c park.c

thread_queue.o: thread_queue.c thread_queue.h
	$(CC) $(CFLAGS) -c thread_queue.c

clean:
	rm -f *.o park