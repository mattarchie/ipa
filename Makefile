CC = gcc
INCFLAGS = -I./ -I./noomr
DEFS = -D NOOMR_ALIGN_HEADERS -D COLLECT_STATS -D NOOMR_SYSTEM_ALLOC
OPT_FLAGS = -O3 -fno-strict-aliasing -fno-strict-overflow
DEBUG_FLAGS = -ggdb3 -g3 -pg
CFLAGS = $(OPT_FLAGS) $(DEBUG_FLAGS) -Wall -Wno-unused-function -Wno-deprecated-declarations -march=native $(INCFLAGS) $(DEFS)
TEST_SOURCE = $(wildcard tests/*.c)
HEADERS = $(wildcard *.h)
TEST_BINARIES = $(basename $(TEST_SOURCE))
TEST_OBJECTS = $(patsubst %.c,%.o, $(TEST_SOURCE))
OBJECTS = noomr.o memmap.o noomr_utils.o
LDFLAGS = -Wl,--no-as-needed -ldl  -rdynamic
LIBRARY = libnoomr.a

default: $(LIBRARY)

$(LIBRARY): $(OBJECTS)
	ar rcs $@ $?
	ranlib $@

memmap.o: memmap.c memmap.h noomr_utils.h
noomr.o: noomr.c noomr.h stack.h memmap.h noomr_utils.h
noomr_utils.o: noomr_utils.c noomr.h noomr_utils.h
tests/%.o: tests/%.c dummy.h noomr.h $(HEADERS)
tests/stack_%: stack.h

# stack tests doesn't depend on the whole system -- special case
tests/stack_%: tests/stack_%.c
	@echo "Linking $@"
	@$(CC) $(CFLAGS) $? -o $@ $(LDFLAGS)

tests/%: tests/%.o $(OBJECTS)
	@echo "Linking $@"
	@$(CC) -rdynamic $(CFLAGS) $< $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.c
	@echo "Compiling $@"
	@$(CC) $(CFLAGS) -c -o $@ $<

tests: $(TEST_OBJECTS) $(TEST_BINARIES)
# NOTE: Tests objects is listed as a dependency so make will not auto-remove them 

test: $(TEST_BINARIES)
	@echo Test binaries: $(notdir $(TEST_BINARIES))
	@ruby tests/test_runner.rb $(TEST_BINARIES)

clean:
	rm -f $(TEST_OBJECTS) $(TEST_BINARIES) $(OBJECTS) gmon.out $(LIBRARY)
