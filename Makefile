ifneq ($(TRAVIS_CI), 1)
	CC = clang
endif

INCFLAGS = -I./ -I./noomr
DEFS = -UNOOMR_ALIGN_HEADERS -DCOLLECT_STATS -DNOOMR_SYSTEM_ALLOC -DNO_HOOK
OPT_FLAGS = -O3 -fno-strict-aliasing -fno-strict-overflow -march=native
DEBUG_FLAGS = -ggdb3 -g3
CFLAGS = $(OPT_FLAGS) $(DEBUG_FLAGS) -fPIC -Wall -Wno-unused-function -Wno-deprecated-declarations -Wno-missing-braces $(INCFLAGS) $(DEFS)
TEST_SOURCE = $(wildcard tests/*.c) $(wildcard tests/parallel/*.c)
SOURCE = $(wildcard *.c)
ALL_SOURCE = $(TEST_SOURCE) $(SOURCE)
HEADERS = $(wildcard *.h)
TEST_BINARIES = $(basename $(TEST_SOURCE))
TEST_OBJECTS = $(patsubst %.c,%.o, $(TEST_SOURCE))
DEP = $(SOURCE:.c=.d) $(TEST_SOURCE:.c=.d)

OBJECTS = noomr.o memmap.o noomr_utils.o noomr_sync.o
LDFLAGS = -Wl,--no-as-needed -ldl -rdynamic -lm
LIBRARY = libnoomr.a

default: $(LIBRARY)

libnoomr.a: $(OBJECTS)
	@echo Building library $@
	@ar rcs $@ $?
	@ranlib $@

%.d: %.c
	@echo "Creating dependency $@"
	@$(CC) $(CFLAGS) -MM -o $@ $?
-include $(DEP)


tests/parallel/stack_%: tests/parallel/stack_%.c
	@echo "Linking $@"
	@$(CC) $(CFLAGS) -pthread $? -o $@ $(LDFLAGS)

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
	@echo Test binaries: $(sort $(notdir $(TEST_BINARIES)))
	ruby tests/test_runner.rb $(sort $(TEST_BINARIES))

clean:
	rm -f $(TEST_OBJECTS) $(TEST_BINARIES) $(OBJECTS) gmon.out $(LIBRARY)
clobber: clean
	rm -f $(DEP)
