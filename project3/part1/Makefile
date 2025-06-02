all: park

park: park.c
	gcc -lpthread -g $^ -o $@

%.o: %.c
	gcc -f $<

clean:
	rm park