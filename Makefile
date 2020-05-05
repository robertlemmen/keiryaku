CFLAGS ?= -std=c11 -D_GNU_SOURCE -pthread -fPIC -Wall -g
LDFLAGS ?= -lm
CC := gcc
TARGET := keiryaku

SOURCES := $(filter-out version.c, $(shell ls *.c))
OBJECTS := $(subst .c,.o,$(SOURCES))

GIT_HASH := $(shell git log -1 --pretty=format:g%h)
GIT_DIRTY := $(shell git describe --all --long --dirty | grep -q dirty && echo 'dirty' || true)
GIT_TAG := $(shell git describe --exact-match 2>/dev/null || true)
VERSION_STRING := $(if $(GIT_TAG),$(GIT_TAG),$(GIT_HASH))$(if $(GIT_DIRTY), (dirty),)

.PHONY: clean test

$(TARGET): $(OBJECTS) linenoise.o
	@cat version.c.template | sed -e 's/\%VERSION_STRING\%/$(VERSION_STRING)/' > version.c
	@echo "Linking $@..."
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ version.c
	@rm -f version.c

%.o: %.c
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) $(CINCFLAGS) -c $<
	@$(CC) -MM $(CFLAGS) $(CINCFLAGS) -c $< > $*.d

linenoise.o: linenoise/linenoise.c
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) $(CINCFLAGS) -c $<
	@$(CC) -MM $(CFLAGS) $(CINCFLAGS) -c $< > $*.d

test: $(TARGET)
	@$(MAKE) --no-print-directory -C check
	@echo
	@echo "Running Scheme Tests..."
	@tdir=`mktemp -d`; \
	failed_count=0; \
	total_count=0; \
	for tf in $$(find t -name "*.t" | sort); do \
		[ -e "$$tf" ] || continue; \
		echo "  $$tf ..."; \
		total_count=`expr $$total_count + 1`; \
		cat $$tf | grep -v "^;" | sed '1,/===/d' > $$tdir/expected; \
		cat $$tf | grep -v "^;" | sed '/===/,$$d' | ./$(TARGET) --debug > $$tdir/result; \
		if [ $$? -ne 0 ]; then \
			echo; \
			echo "test failed to execute: $$tf"; \
			echo; \
			failed_count=`expr $$failed_count + 1`; \
		else \
			diff -uB $$tdir/expected /$$tdir/result > $$tdir/diff || true; \
			if [ -s $$tdir/diff ]; then \
				echo; \
				echo "difference in test $$tf:"; \
				cat $$tdir/diff; \
				echo; \
				failed_count=`expr $$failed_count + 1`; \
			fi; \
		fi; \
		rm -f $$tdir/expected $$tdir/result $$tdir/diff; \
	done; \
	rm -rf $$tdir; \
	if [ $$failed_count -gt 0 ]; then \
		echo; \
		echo "$$failed_count / $$total_count tests failed!"; \
		exit 1; \
	fi; \
	echo "all $$total_count tests passed ok!"

clean:
	@echo "Cleaning..."
	@rm -f *.o *.d
	@rm -f version.c
	@rm -f $(TARGET)
	@$(MAKE) --no-print-directory -C check clean

-include *.d
