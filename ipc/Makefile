CC = gcc
CFLAGS = -Wall -Wextra
OBJ = msg_test.o

msg_test: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@

msg_test.o: msg_test.c

.PHONY: clean
clean:
	rm msg_test $(OBJ)
