CC=gcc
CFLAGS = -Wall -Wextra -g
serial: img.h img.c imgio.c
	$(CC) img.c imgio.c -o imgserial $(CFLAGS)
omp: img.h imgomp.c imgio.c
	$(CC) -fopenmp imgomp.c imgio.c -o imgomp $(CFLAGS)
pthreads: img.h imgpthr.c imgio.c
	$(CC) imgpthr.c imgio.c -o imgpthr $(CFLAGS) -lpthread
clean:
	rm -f imgserial imgomp imgpthr
	rm -f *.ppm