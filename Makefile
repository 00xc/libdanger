CFLAGS = -Wall -Wextra -Wpedantic -O3 -std=c89 -Iinclude/
TEST_FLAGS = -fsanitize=address,undefined

OBJS = dngr_domain.o dngr_list.o
STATIC_LIB = libdanger.a

TESTS = tests/test_list
EXAMPLES = examples/thread_prog

.PHONY: all static clean tests examples

%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $^

all: static

static: $(STATIC_LIB)

$(STATIC_LIB): $(OBJS)
	ar rcs $(STATIC_LIB) $?

tests: $(TESTS)
	./tests/test_list

tests/test_list: tests/test_list.c $(STATIC_LIB)
	$(CC) $^ -o $@ $(CFLAGS) $(TEST_FLAGS)

examples: $(EXAMPLES)

examples/thread_prog: examples/thread_prog.c $(STATIC_LIB)
	$(CC) $^ -o $@ $(CFLAGS) $(TEST_FLAGS) -pthread

clean:
	rm -f $(OBJS)
	rm -f $(STATIC_LIB)
	rm -f $(TESTS)
	rm -f $(EXAMPLES)
