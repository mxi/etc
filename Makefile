.PHONY: all clean

CFLAGS := -std=c99                \
		  -Wall -Wextra -pedantic \
		  -Wold-style-definition  \

ifdef optimize
	CFLAGS += -O2         \
			  -ffast-math \
			  -DNDEBUG=1

else
	CFLAGS += -O0 -ggdb            \
			  -fsanitize=undefined \
			  -fsanitize=address
endif

# terminfo
load_terminfo: load_terminfo.c external/basics.h
	$(CC) $(CFLAGS) -o $@ $<

TARGETS += load_terminfo

# PHONYs
all: $(TARGETS)

clean:
	@-for artifact in $(TARGETS); do  \
		[ -e "$$artifact" ]           \
		&& echo "rm -rf $$artifact"   \
		&& rm -rf "$$artifact"        \
		|| continue;                  \
	done
