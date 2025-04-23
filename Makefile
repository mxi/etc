SOURCES := $(wildcard *.c)
TARGETS := $(SOURCES:%.c=%)

CFLAGS := -std=c99 -Wall -Wextra -Wold-style-definition -pedantic \
	      -O0 -g

all: $(TARGETS)

clean:
	rm -rf $(TARGETS)

$(TARGETS): %: %.c Makefile
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: all clean
