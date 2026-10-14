CC = clang
CFLAGS = -Wall -Wextra -g -std=c23
TARGET = main
SOURCES = main.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SOURCES) Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) $(LDFLAGS) $(LDLIBS) -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
	rm -rf $(TARGET).dSYM
