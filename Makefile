CC = gcc
INCFLAGS = -I./ -I./noomr
DEFS = -D NOOMR_ALIGN_HEADERS -D COLLECT_STATS -D NOOMR_SYSTEM_ALLOC
OPT_FLAGS = -O0
DEBUG_FLAGS = -ggdb3 -g3
CFLAGS = $(OPT_FLAGS) $(DEBUG_FLAGS) -Wall -Wno-unused-function -Wno-deprecated-declarations -march=native $(INCFLAGS) $(DEFS)
TEST_BINARIES = $(basename $(wildcard tests/*.c))
OBJECTS = noomr.o memmap.o noomr_utils.o
LDFLAGS = -Wl,--no-as-needed -lm -ldl -static -rdynamic
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
	@echo "Linking $@"
	@$(CC) $(CFLAGS) $? -o $@ $(LDFLAGS)

tests/%: tests/%.c $(OBJECTS) tests/dummy.h
	@echo "Linking $@"
	@$(CC) $(CFLAGS) $? -o $@ $(LDFLAGS)

%.o: %.c
	@echo "Compiling $@"
	@$(CC) -rdynamic $(CFLAGS) -c -o $@ $<

tests: $(TEST_BINARIES)


test: $(TEST_BINARIES)
	@echo Test binaries: $(notdir $(TEST_BINARIES))
	@ruby tests/test_runner.rb $(TEST_BINARIES)

clean:
	rm -f tests/*.o $(TEST_BINARIES) $(OBJECTS) gmon.out $(LIBRARY)
