CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -O2 -ICore

TARGET := build/dummy_test
CORE_SRC := Core/ring_buffer.c
TEST_SRC := test/dummy_test.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(CORE_SRC) $(TEST_SRC)
	@mkdir -p build
	$(CC) $(CFLAGS) $(TEST_SRC) $(CORE_SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build
