CC = gcc
CFLAGS = -O0 -ggdb3 -ggdb -g  -Wall -march=native

stack.o: stack.c stack.h

clean:
	rm -f *.o
