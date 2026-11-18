# Mandelbrot

## Profile Timing

Interesting: `pi3b+` is actually faster than `pi4` for `Mandelbrot`.
Command:

    > time sudo ./obj/debug/bin/Mandelbrot -grey -nmax

_20261118. All times in seconds. Comparing single runs. Warden average from logging of Global timers._

|----------|---------------------|-----------------|-----------|----------------|
| Platform | Kernel Compile Time | Kernel run Time | Real Time | Warden average |
|----------|---------------------|-----------------|-----------|----------------|
|    pi3b+ |           0.385392s |       0.285040s |    1.931s |         1.280s |
|    pi4   |           0.676695s |       0.247948s |    3.288s |         1.607s |
|    pi5   |           0.146413s |       0.067935s |    0.524s |         0.621s |
|----------|---------------------|-----------------|-----------|----------------|

- The difference is mainly in the compile time, although the run time of `pi3b+`
  competes quite well with `pi4`.
  Perhaps I _should_ further pursue precompiled headers.
