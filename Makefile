CC=gcc
CFLAGS = -Wall -Wextra -g
all: serial omp pthreads hybrid
serial: img.h img.c imgio.c
	$(CC) img.c imgio.c -o imgserial $(CFLAGS)
omp: img.h imgomp.c imgio.c
	$(CC) -fopenmp imgomp.c imgio.c -o imgomp $(CFLAGS)
mpi: img.h imgmpi.c imgio.c
	mpicc imgmpi.c imgio.c -o imgmpi $(CFLAGS)
pthreads: img.h imgpthr.c imgio.c
	$(CC) imgpthr.c imgio.c -o imgpthr $(CFLAGS) -lpthread
#hybrid: img.h imghybr.c imgio.c
#	$(CC) -fopenmp imghybr.c imgio.c -o imghybr $(CFLAGS) -lpthread
clean:
	rm -f imgserial imgmpi imgomp imgpthr imghybr
	rm -f *.ppm
