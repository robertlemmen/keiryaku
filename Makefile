CFLAGS ?= -std=c11 -D_GNU_SOURCE -pthread -Wall -g
CC := gcc
TARGET := keiryaku

SOURCES := $(shell ls *.c)
OBJECTS := $(subst .c,.o,$(SOURCES))

GIT_HASH := $(shell git log -1 --pretty=format:g%h)
GIT_DIRTY := $(shell git describe --all --long --dirty | grep -q dirty && echo 'dirty' || true)
GIT_TAG := $(shell git describe --exact-match 2>/dev/null || true)
VERSION_STRING := $(if $(GIT_TAG),$(GIT_TAG),$(GIT_HASH))$(if $(GIT_DIRTY), (dirty),)

.PHONY: clean test version.c

$(TARGET): $(OBJECTS) version.o
	@echo "Linking $@..."
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

version.c:
	@echo "Creating $@..."
	@VERSION_STRING=$$GIT_TAG; \
	 if [ ! -z "$$GIT_TAG" ]; \
	 then \
	     VERSION_STRING=$$GIT_TAG; \
	 fi; \
	 export VERSION_STRING=test234; \
	 cat version.c.template | sed -e 's/\%VERSION_STRING\%/$(VERSION_STRING)/' > version.c

%.o: %.c
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) $(CINCFLAGS) -c $<
	@$(CC) -MM $(CFLAGS) $(CINCFLAGS) -c $< > $*.d

version.o: version.c

test: $(TARGET)
	@echo "Running Tests..."
	@failed_count=0; \
	for tf in $$(find t -name "*.t" | sort); do \
		[ -e "$$tf" ] || continue; \
		echo "  $$tf ..."; \
		cat $$tf | grep -v "^;" | sed '/===/,$$d' | ./$(TARGET) --debug > /tmp/result; \
		cat $$tf | grep -v "^;" | sed '1,/===/d' > /tmp/expected; \
		diff -uB /tmp/expected /tmp/result > /tmp/diff || true; \
		if [ -s /tmp/diff ]; then \
			echo; \
			echo "difference in test $$tf:"; \
			cat /tmp/diff; \
			echo; \
			rm -f /tmp/expected /tmp/result /tmp/diff; \
			failed_count=`expr $$failed_count + 1`; \
		fi; \
	done; \
	if [ $$failed_count -gt 0 ]; then \
		echo; \
		echo "$$failed_count tests failed!"; \
		exit 1; \
	fi; \
	echo; \
	echo "all ok!"

clean:
	@echo "Cleaning..."
	@rm -f *.o *.d
	@rm -f version.c
	@rm -f $(TARGET)

-include *.d
