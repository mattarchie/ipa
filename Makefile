CC = gcc
INCFLAGS = -I./
CFLAGS = -O3 -ggdb3 -ggdb -g -ffast-math -Wall -Wno-unused-function -march=native $(INCFLAGS) -D NOOMR_ALIGN_HEADERS -D COLLECT_STATS -pthread
TEST_BINARIES = tests/stack_test
OBJECTS = stack.o noomr.o
LDFLAGS = -lm

default: $(OBJECTS)

stack.o: stack.c stack.h
memmap.o: memmap.c memmap.h
noomr.o: noomr.c noomr.h stack.o memmap.o
noomr: noomr.o stack.o

tests/stack_test: tests/stack_test.c stack.o

test: $(TEST_BINARIES)
	./tests/stack_test

clean:
	rm -f *.o tests/*.o $(TEST_BINARIES) $(OBJECTS)
