CC = gcc
INCFLAGS = -I./
CFLAGS = -O3 -ggdb3 -ggdb -g -ffast-math -Wall -Wno-unused-function -march=native $(INCFLAGS) -D NOOMR_ALIGN_HEADERS -D COLLECT_STATS
TEST_BINARIES = tests/stack_test
OBJECTS = noomr.o memmap.o
LDFLAGS = -lm

default: $(OBJECTS)

memmap.o: memmap.c memmap.h
noomr.o: noomr.c noomr.h stack.h memmap.h

tests/stack_test: tests/stack_test.c stack.h

tests/%:
	$(CC) $(CFLAGS) $? -o $@ $(LDFLAGS)

test: $(TEST_BINARIES)
	$(foreach tb, $?, ./$(tb))

clean:
	rm -f tests/*.o $(TEST_BINARIES) $(OBJECTS)
