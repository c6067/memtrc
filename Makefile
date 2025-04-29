# Compiler settings
CC = gcc
CFLAGS = -Iinclude -pthread -Wall -Wextra -g -D_POSIX_C_SOURCE=200809L
DEPS = $(wildcard include/*.h)
OBJ = memtrc.o chart.o
TEST_OBJ = test.o $(OBJ)
TARGET = memtrc
TEST_TARGET = test

.PHONY: all clean test build-test debug release

# Build targets
all: release

release: CFLAGS += -O2
release: $(TARGET)

debug: CFLAGS += -g -O0 -DDEBUG
debug: $(TARGET)

# Main program build
$(TARGET): main.c $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) -lm

# Test program build
$(TEST_TARGET): $(TEST_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) -lm

# Run tests
build-test: $(TEST_TARGET)
	./$(TEST_TARGET)

# Pattern rule for object files
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

# Cleanup
clean:
	rm -f *.o $(TARGET) $(TEST_TARGET) *.log *.out


# append Install and uninstall
.PHONY: install uninstall

install: release
	install -m 0755 $(TARGET) /usr/local/bin/

uninstall:
	rm -f /usr/local/bin/$(TARGET)


