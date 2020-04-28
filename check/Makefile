CFLAGS ?= -std=c11 -Wall -g $(shell pkg-config --cflags check) -I..
LDFLAGS := $(shell pkg-config --libs check)
CC := gcc
TARGET := keiryaku_check

SOURCES := $(shell ls *.c)
OBJECTS := $(subst .c,.o,$(SOURCES))
TESTED_OBJECTS=../heap.o ../types.o ../global.o

.PHONY: clean check

check: $(TARGET)
	@echo "Running Unit Tests..."
	@./$(TARGET)

$(TARGET): $(OBJECTS) $(TESTED_OBJECTS)
	@echo "Linking $@..."
	@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) $(CINCFLAGS) -c $<
	@$(CC) -MM $(CFLAGS) $(CINCFLAGS) -c $< > $*.d

clean:
	@echo "Cleaning unit tests..."
	@rm -f *.o *.d
	@rm -f $(TARGET)

-include *.d
