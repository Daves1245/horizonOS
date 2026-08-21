# -idirafter so libc's freestanding headers don't shadow the host's when
# building this demo natively; it only needs to find <dsa/list.h>.
all:
	gcc -Wall -idirafter ../../../include main.c -o run
clean:
	rm run
