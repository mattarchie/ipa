CC = gcc
INCFLAGS = -I./
CFLAGS = -O0 -ggdb3 -ggdb -g  -Wall -march=native $(INCFLAGS)
TEST_BINARIES = tests/stack_test

stack.o: stack.c stack.h

tests/stack_test: tests/stack_test.c stack.o

test: $(TEST_BINARIES)
	./tests/stack_test

clean:
	rm -f *.o tests/*.o $(TEST_BINARIES)
