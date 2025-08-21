CFLAGS := -std=c99 -Wall -Wextra -Wold-style-definition -pedantic \
	      -O0 -g

a.out: main.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -r $(TARGETS)

.PHONY: clean
