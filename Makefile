CC := gcc
CFLAGS := -lm -Wall -Wextra -g
SRC := main.c

cache-sim: $(SRC)
	$(CC) $(CFLAGS) -o cache-sim $(SRC)

clean:
	rm -f cache-sim

.PHONY: all clean
