BUILD_DIR := build
SOURCES := $(wildcard *.c)
TARGETS := $(SOURCES:%.c=$(BUILD_DIR)/%)

CFLAGS := -std=c99 -Wall -Wextra -Wold-style-definition -pedantic \
	      -O0 -g

all: $(TARGETS)

clean:
	rm -rf $(BUILD_DIR)

$(TARGETS): $(BUILD_DIR)/%: %.c Makefile
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: all clean
