# Image Filters

### Loging on university queue
```
ssh user.name@fep.grid.pub.ro
use nehalem queue as explained in: http://cs.curs.pub.ro/wiki/asc/_media/asc:resurse:cluster-cheat-sheet.pdf
```

### Fep Grid Results:

POSTERIZE
Serial running time is: 2.530000

PIXELATE
Serial running time is: 1.860000

***********************************************************

POSTERIZE
OMP running time for 1 threads is: 2.905003
OMP running time for 2 threads is: 1.524588
OMP running time for 4 threads is: 1.045683
OMP running time for 8 threads is: 0.726569
OMP running time for 16 threads is: 0.493826

PIXELATE
OMP running time for 1 threads is: 1.965116
OMP running time for 2 threads is: 1.437738
OMP running time for 4 threads is: 0.804808
OMP running time for 8 threads is: 0.602843
OMP running time for 16 threads is: 0.417507

***********************************************************

POSTERIZE
Pthreads running time for 1 threads is: 3.115354
Pthreads running time for 2 threads is: 3.081623
Pthreads running time for 4 threads is: 1.522332
Pthreads running time for 8 threads is: 0.980817
Pthreads running time for 16 threads is: 0.589198

PIXELATE
Pthreads running time for 1 threads is: 2.616622
Pthreads running time for 2 threads is: 2.568914
Pthreads running time for 4 threads is: 1.120810
Pthreads running time for 8 threads is: 0.800486
Pthreads running time for 16 threads is: 0.474274

***********************************************************

POSTERIZE
Hybrid running time for 1 threads is: 0.427952
Hybrid running time for 2 threads is: 0.514690
Hybrid running time for 4 threads is: 0.528134
Hybrid running time for 8 threads is: 0.541748
Hybrid running time for 16 threads is: 0.726876

PIXELATE
Hybrid running time for 1 threads is: 0.392366
Hybrid running time for 2 threads is: 0.498047
Hybrid running time for 4 threads is: 0.479092
Hybrid running time for 8 threads is: 0.518967
Hybrid running time for 16 threads is: 0.727811
