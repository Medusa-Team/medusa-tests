CC = gcc
CFLAGS = -Wall -Wextra -D_GNU_SOURCE
PROGS = msg_test sem_test

all: $(PROGS)

$(PROGS): %: %.c
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -f $(PROGS)
