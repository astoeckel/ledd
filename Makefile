%PHONY: clean

all: ledd

ledd: main.c
	cc -Wall -Wextra -pedantic -Wc++-compat -std=c99 main.c -o ledd

clean:
	rm -f ledd
