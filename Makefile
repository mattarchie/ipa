CC = gcc
INCFLAGS = -I./
DEFS = -D NOOMR_ALIGN_HEADERS -D COLLECT_STATS -D NOOMR_SYSTEM_ALLOC
OPT_FLAGS = -O0
DEBUG_FLAGS = -pg -ggdb3 -ggdb -g3
CFLAGS = $(OPT_FLAGS) $(DEBUG_FLAGS) -Wall -Wno-unused-function -Wno-deprecated-declarations -fPIC -march=native $(INCFLAGS) $(DEFS)
TEST_BINARIES = $(basename $(wildcard tests/*.c))
OBJECTS = noomr.o memmap.o
LDFLAGS = -Wl,--no-as-needed -lm -ldl -static
LIBRARY = noomr.so

default: $(OBJECTS)
library: $(LIBRARY)

memmap.o: memmap.c memmap.h
noomr.o: noomr.c noomr.h stack.h memmap.h
tests/stack_%: stack.h


noomr.so: $(OBJECTS)
	$(CC) -o $@ $? -shared

# stack tests doesn't depend on the whole system -- special case
tests/stack_%: tests/stack_%.c
	$(CC) $(CFLAGS) $? -o $@ $(LDFLAGS)

tests/%: tests/%.c $(OBJECTS)
	$(CC) $(CFLAGS) $? -o $@ $(LDFLAGS)

tests: $(TEST_BINARIES)

test: $(TEST_BINARIES)
	@ruby tests/test_runner.rb $(TEST_BINARIES)

clean:
	rm -f tests/*.o $(TEST_BINARIES) $(OBJECTS)
