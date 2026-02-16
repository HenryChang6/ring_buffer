CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -O2 -ICore

CORE_SRC := Core/ring_buffer.c
BOUNDARY_TARGET := build/boundary_test
WRAP_DROP_TARGET := build/wrap_drop_test
INTEG_TARGET := build/integration_test
BOUNDARY_SRC := test/boundary_test.c
WRAP_DROP_SRC := test/wrap_drop_test.c
INTEG_SRC := test/integration_test.c

.PHONY: all run-boundary run-wrap-drop run-integration test clean

all: $(BOUNDARY_TARGET) $(WRAP_DROP_TARGET) $(INTEG_TARGET)

$(BOUNDARY_TARGET): $(CORE_SRC) $(BOUNDARY_SRC)
	@mkdir -p build
	$(CC) $(CFLAGS) $(BOUNDARY_SRC) $(CORE_SRC) -o $(BOUNDARY_TARGET)

$(WRAP_DROP_TARGET): $(CORE_SRC) $(WRAP_DROP_SRC)
	@mkdir -p build
	$(CC) $(CFLAGS) $(WRAP_DROP_SRC) $(CORE_SRC) -o $(WRAP_DROP_TARGET)

$(INTEG_TARGET): $(CORE_SRC) $(INTEG_SRC)
	@mkdir -p build
	$(CC) $(CFLAGS) -pthread $(INTEG_SRC) $(CORE_SRC) -o $(INTEG_TARGET)

run-boundary: $(BOUNDARY_TARGET)
	./$(BOUNDARY_TARGET)

run-wrap-drop: $(WRAP_DROP_TARGET)
	./$(WRAP_DROP_TARGET)

run-integration: $(INTEG_TARGET)
	./$(INTEG_TARGET)

test: run-boundary run-wrap-drop run-integration

clean:
	rm -rf build
