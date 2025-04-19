EXEC=pseudoshell
FLAGS=-g -o
CC=gcc

all: $(EXEC)

$(EXEC): main.o string_parser.o command.o
	$(CC) $(FLAGS) $@ $^

main.o: main.c
	$(CC) -c $^

%.o: %.c %.h
	$(CC) -c $<

# string_parser.o: string_parser.c string_parser.h
# 	gcc -c $<   # $< grabs left dependency

# command.o: command.c command.h
# 	gcc -c $<

clean:
	rm -f *.o $(EXEC)