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

load_terminfo: load_terminfo.c
	$(CC) $(CFLAGS) -o $@ $<

TARGETS += load_terminfo

all: $(TARGETS)

clean:
	@-for artifact in $(TARGETS); do  \
		[ -e "$$artifact" ]           \
		&& echo "rm -rf $$artifact"   \
		&& rm -rf "$$artifact"        \
		|| continue;                  \
	done
