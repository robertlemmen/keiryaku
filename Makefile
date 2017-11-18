CFLAGS=-std=c11 -D_GNU_SOURCE -pthread -Wall -g
CC=gcc
TARGET=kenbou

SOURCES=$(shell ls *.c)
OBJECTS=$(subst .c,.o,$(SOURCES))

.PHONY: clean 

$(TARGET): $(OBJECTS)
	@echo "Linking $@..."
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.o: %.c
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) $(CINCFLAGS) -c $<
	@$(CC) -MM $(CFLAGS) $(CINCFLAGS) -c $< > $*.d

clean:
	@echo "Cleaning..."
	@rm -f *.o *.d
	@rm -f $(TARGET)

-include *.d
