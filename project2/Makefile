all: iobound cpubound part1 part2 part3

iobound:
	gcc -g iobound.c -o iobound

cpubound:
	gcc -g cpubound.c -o cpubound

part1: part1.c
	gcc -g part1.c -o part1

part2: part2.c
	gcc -g part2.c -o part2

part3: part3.c
	gcc -g part3.c -o part3

clean:
	rm -rf *.dSYM iobound cpubound part1 part2 part3