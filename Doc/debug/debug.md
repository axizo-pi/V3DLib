# Add` gdb` breakpoint to code

```
#include <csignal>          // std::raise(SIGINT);
...
	std::raise(SIGINT);
```

# GCD hangs on pi4 (vc6)

GCD gives following error output:

```
ERROR: ERROR v3d_wait_bo(): Timer expired

terminate called after throwing an instance of 'V3DLib::Exception'
  what():  Assertion failed: v3d_wait_bo(): call iotctl failed
Aborted
```

After this, other example programs will also hang in the same manner.

Eventually, processes will hang and a hard reset is needed.


- Irrespective of mesa1 and mesa2
- works fine on pi3 (vc4)
- All other example programs work, until GCD is run.

Hypothesis:

Some difference in assembly output.
**TODO** - compare vc4 and vc6 assembly
