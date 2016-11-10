CC = gcc
INCFLAGS = -I./ -I./noomr
DEFS = -D NOOMR_ALIGN_HEADERS -D COLLECT_STATS -D NOOMR_SYSTEM_ALLOC
OPT_FLAGS = -O0
DEBUG_FLAGS = -ggdb3 -g
CFLAGS = $(OPT_FLAGS) $(DEBUG_FLAGS) -Wall -Wno-unused-function -Wno-deprecated-declarations -march=native $(INCFLAGS) $(DEFS)
TEST_BINARIES = $(basename $(wildcard tests/*.c))
OBJECTS = noomr.o memmap.o noomr_utils.o
LDFLAGS = -Wl,--no-as-needed -lm -ldl -static
LIBRARY = libnoomr.a

default: $(LIBRARY)

$(LIBRARY): $(OBJECTS)
	ar rcs $@ $?
	ranlib $@

memmap.o: memmap.c memmap.h noomr_utils.h
noomr.o: noomr.c noomr.h stack.h memmap.h noomr_utils.h
noomr_utils.o: noomr_utils.c noomr.h noomr_utils.h
tests/stack_%: stack.h

# stack tests doesn't depend on the whole system -- special case
tests/stack_%: tests/stack_%.c
	$(CC) $(CFLAGS) $? -o $@ $(LDFLAGS)

tests/%: tests/%.c $(OBJECTS) tests/dummy.h
	$(CC) $(CFLAGS) $? -o $@ $(LDFLAGS)

tests: $(TEST_BINARIES)

foo: $(OBJECTS)

test: $(TEST_BINARIES)
	@echo Test binaries: $(notdir $(TEST_BINARIES))
	@ruby tests/test_runner.rb $(TEST_BINARIES)

clean:
	rm -f tests/*.o $(TEST_BINARIES) $(OBJECTS) gmon.out
