# Image Filters

### Loging on university queue
```

ssh user.name@fep.grid.pub.ro
use nehalem queue as explained in: http://cs.curs.pub.ro/wiki/asc/_media/asc:resurse:cluster-cheat-sheet.pdf
```

### Fep Grid Results:

POSTERIZE

* Serial running time is: 2.530000


PIXELATE

* Serial running time is: 1.860000


***********************************************************

POSTERIZE

* OMP running time for 1 threads is: 2.905003
* OMP running time for 2 threads is: 1.524588
* OMP running time for 4 threads is: 1.045683
* OMP running time for 8 threads is: 0.726569
* OMP running time for 16 threads is: 0.493826


PIXELATE

* OMP running time for 1 threads is: 1.965116
* OMP running time for 2 threads is: 1.437738
* OMP running time for 4 threads is: 0.804808
* OMP running time for 8 threads is: 0.602843
* OMP running time for 16 threads is: 0.417507

***********************************************************

POSTERIZE

* Pthreads running time for 1 threads is: 3.115354
* Pthreads running time for 2 threads is: 3.081623
* Pthreads running time for 4 threads is: 1.522332
* Pthreads running time for 8 threads is: 0.980817
* Pthreads running time for 16 threads is: 0.589198


PIXELATE

* Pthreads running time for 1 threads is: 2.616622
* Pthreads running time for 2 threads is: 2.568914
* Pthreads running time for 4 threads is: 1.120810
* Pthreads running time for 8 threads is: 0.800486
* Pthreads running time for 16 threads is: 0.474274

***********************************************************

POSTERIZE
* MPI running time is: 0.178786
* MPI running time is: 16.681595
* MPI running time is: 14.639783
* MPI running time is: 43.927502
* MPI running time is: 69.559403

PIXELATE
* MPI running time is: 17.273602
* MPI running time is: 13.735005
* MPI running time is: 50.632013
* MPI running time is: 56.459588

***********************************************************

POSTERIZE
* MPI-OMP running time is: 15.094850
* MPI-OMP running time is: 18.839990
* MPI-OMP running time is: 33.000534
* MPI-OMP running time is: 64.260157

PIXELATE
* MPI-OMP running time is: 15.537996
* MPI-OMP running time is: 15.800323
* MPI-OMP running time is: 42.949154
* MPI-OMP running time is: 65.900048
