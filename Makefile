CC = gcc
INCFLAGS = -I./
DEFS = -D NOOMR_ALIGN_HEADERS -D COLLECT_STATS -D NOOMR_SYSTEM_ALLOC
CFLAGS = -O0 -fno-inline -pg -ggdb3 -ggdb -g -fPIC -ffast-math -Wall -Wno-unused-function -march=native $(INCFLAGS)
TEST_BINARIES = tests/test_allocate tests/override
OBJECTS = noomr.o memmap.o
LDFLAGS = -ldl -lm

default: $(OBJECTS)

memmap.o: memmap.c memmap.h
noomr.o: noomr.c noomr.h stack.h memmap.h

tests/stack_test: tests/stack_test.c stack.h
tests/test_allocate: tests/test_allocate.c $(OBJECTS)
tests/override: tests/override.c

tests/%:
	$(CC) $(CFLAGS) $? -o $@ $(LDFLAGS)

test: $(TEST_BINARIES)
	@$(foreach tb, $?, echo ./$(tb) && ./$(tb); )

clean:
	rm -f tests/*.o $(TEST_BINARIES) $(OBJECTS)
