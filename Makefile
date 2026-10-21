CC = clang
CFLAGS = -Wall -Wextra -g -std=c23
TARGET = main
SOURCES = main.c
TEST_TARGET = tests/editor_test

.PHONY: all run test clean

all: $(TARGET)

$(TARGET): $(SOURCES) Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) $(LDFLAGS) $(LDLIBS) -o $@

run: $(TARGET)
	./$(TARGET)

$(TEST_TARGET): tests/editor_test.c $(SOURCES) Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/editor_test.c $(LDFLAGS) $(LDLIBS) -o $@

test: $(TARGET) $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(TARGET) $(TEST_TARGET)
	rm -rf $(TARGET).dSYM
	rm -rf $(TEST_TARGET).dSYM
