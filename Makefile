CC=gcc
CFLAGS = -Wall -Wextra -g
build: img.h img.c
	$(CC) img.c -o imgserial $(CFLAGS)
clean:
	rm imgserial
