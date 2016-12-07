CC=gcc
CFLAGS = -Wall -Wextra -g
serial: img.h img.c
	$(CC) img.c -o imgserial $(CFLAGS)
omp: img.h imgomp.c
	$(CC) -fopenmp imgomp.c -o imgomp $(CFLAGS)
clean:
	rm -f imgserial imgomp imgmpi
	rm -f *.ppm
