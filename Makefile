.PHONY: all clean

all: ledd

ledd: main.c
	cc -Wall -Wextra -pedantic -Wc++-compat -std=c99 main.c -O3 -s -o ledd

clean:
	rm -f ledd
