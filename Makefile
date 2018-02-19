CFLAGS=-std=c11 -D_GNU_SOURCE -pthread -Wall -g
CC=gcc
TARGET=keiryaku

SOURCES=$(shell ls *.c)
OBJECTS=$(subst .c,.o,$(SOURCES))

.PHONY: clean test

$(TARGET): $(OBJECTS)
	@echo "Linking $@..."
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.o: %.c
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) $(CINCFLAGS) -c $<
	@$(CC) -MM $(CFLAGS) $(CINCFLAGS) -c $< > $*.d

test: $(TARGET)
	@echo "Running Tests..."
	@for tf in *.t; do \
		[ -e "$$tf" ] || continue; \
		echo "  $$tf ..."; \
		cat $$tf | grep -v "^;" | sed '/===/,$$d' | ./$(TARGET) > /tmp/result; \
		cat $$tf | grep -v "^;" | sed '1,/===/d' > /tmp/expected; \
		diff -uB /tmp/expected /tmp/result > /tmp/diff || true; \
		if [ -s /tmp/diff ]; then \
			echo "difference in test $$tf:"; \
			cat /tmp/diff; \
			rm -f /tmp/expected /tmp/result /tmp/diff; \
			exit 1; \
		fi; \
	done;
	@rm -f /tmp/expected /tmp/result /tmp/diff
	@echo "all ok!"

clean:
	@echo "Cleaning..."
	@rm -f *.o *.d
	@rm -f $(TARGET)

-include *.d
