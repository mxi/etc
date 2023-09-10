.PHONY: all clean

CFLAGS := -std=c99                \
		  -Wall -Wextra -pedantic \
		  -Wold-style-definition  \
		  -fsanitize=undefined    \
		  -fsanitize=address

ifdef optimize
	CFLAGS += -O2 -ffast-math
else
	CFLAGS += -O0 -ggdb
endif

TARGETS := load_terminfo

$(TARGETS): %: %.c
	$(CC) $(CFLAGS) -o $@ $<

all: $(TARGETS)

clean:
	@-for artifact in $(TARGETS); do  \
		[ -e "$$artifact" ]           \
		&& echo "rm -rf $$artifact"   \
		&& rm -rf "$$artifact"        \
		|| continue;                  \
	done
