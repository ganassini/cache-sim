CC := gcc
CFLAGS := -lm -Wall -Wextra
SRC = main.c util.c

cache-sim: $(SRC)
	$(CC) $(CFLAGS) -o cache-sim $(SRC) -I .

clean:
	rm -f cache-sim

.PHONY: all clean
