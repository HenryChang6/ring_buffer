CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -O2 -ICore

CORE_SRC := Core/ring_buffer.c
DUMMY_TARGET := build/dummy_test
INTEG_TARGET := build/integration_test
DUMMY_SRC := test/dummy_test.c
INTEG_SRC := test/integration_test.c

.PHONY: all run run-dummy run-integration clean

all: $(DUMMY_TARGET) $(INTEG_TARGET)

$(DUMMY_TARGET): $(CORE_SRC) $(DUMMY_SRC)
	@mkdir -p build
	$(CC) $(CFLAGS) $(DUMMY_SRC) $(CORE_SRC) -o $(DUMMY_TARGET)

$(INTEG_TARGET): $(CORE_SRC) $(INTEG_SRC)
	@mkdir -p build
	$(CC) $(CFLAGS) -pthread $(INTEG_SRC) $(CORE_SRC) -o $(INTEG_TARGET)

run: run-dummy

run-dummy: $(DUMMY_TARGET)
	./$(DUMMY_TARGET)

run-integration: $(INTEG_TARGET)
	./$(INTEG_TARGET)

clean:
	rm -rf build
