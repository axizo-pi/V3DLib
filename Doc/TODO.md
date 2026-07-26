<head>
  <link rel="stylesheet" type="text/css" href="css/docs.css">
</head>

# TODO

## Current

- [ ] Use external memory-mapped buffer objects.
- [ ] Fix emulator, errors occur during execution.  
      **TODO:** Find kernels in unit tests that fail (`testConvert.cpp`?).


### v3d

- [ ] Has the timeout hang been fixed yet in the kernel driver? [Check](https://github.com/raspberrypi/linux/pull/3816) from time to time

### Other

- [ ] Scheduling of kernels - see VideoCore `fft` project.
      Done for vc4. v3d is trickier.

### Ideas for Examples

- [ ] [Colliding Galaxies](https://github.com/rtoal/galaxies/tree/master) - javascript implementation
- [ ] Implement [Raytracing](https://gabrielgambetta.com/computer-graphics-from-scratch/02-basic-raytracing.html).


---------------------------
## Previous

### General

- [ ] Initializing a Float/Complex(/Int?) variable without value may not add variable to target code.
      This is a consequence of fixing liveness allocation for dst vars in  conditional instructions.
      Examine, report, prevent, fix.
- [ ] Check in interpreter/emulator for writing out of shared array bounds


### v3d

- [ ] Figure out when and how `sig_magic` and `sig_addr` are used.
      Clues: `mesa/src/compiler/vir_to_qpu.c`, `mesa/src/broadcom/qpu/qpu_disasm.c`
- [ ] figure out what performance the counters signify on `v3d`


### vc4

- [ ] Warning (at least)  in interpreter/emulator for 24-bit multiply overflow
- [ ] Consider using device driver interface for vc4 - this will get rid of need for `sudo`
- [ ] Enforce acc4 (r4) as a read-only register, notably in emulator
- [ ] Enforce non-usage of acc4 (r4) during sfu-call, notably in emulator


### Compile source code

- [ ] Check on rotate value in `rotate()`. Ideally this should be between -15..15 inclusive.


### Unfixable Issue

*This can not be fixed - just keep it in mind*

Example:

    Float x = freq*(x + toFloat(index() - offset));  // Note usage x in RHS (redacted from original)

**Research:**

The issue here is that the following is allowed by `C++` syntax:

    int x = x;  // or any other rhs with x

...and this is also valid for `Int x`. With `-Wall`, you will get output:

    warning: ‘x’ may be used uninitialized in this function [-Wmaybe-uninitialized]

In the case of `Int x = x` the compiler will happily compile, but the contents of `x` on the rhs
are uninitialized and therefore garbage. Due to this, things likely explode on execution.

### Documentation

- [ ] Explain code generation, not direct execution
- [ ] Mailbox functions link to reference and explanation two size fields
- [ ] DSL: Document use of 'Expr'-constructs (e.g. `BoolExpr`) as a kind of lambda


### Unit Tests

- [ ] Adjust emulator so it rounds downward like the hardware QPU's.
  Due to kernel rounding downward for floating point operations, unit tests comparing outputs
  in an emulator-only (QPU=0) build will fail. E.g.:

       Tests/testRot3D.cpp:33: FAILED:
       REQUIRE( y1[i] == y2[i] )
       with expansion:
         -19183.95117f == 19184.0f
       with message:
         Comparing Rot3D_2 for index 19184

  This error happens twice, for `testRot3D`.

### Library Code
- [ ] Add check in emulator for too many `gather()` calls. Or not enough `receive()` calls, same thing
- [ ] Add method for build/platform info, for display on startup of an application


### Ideas for Examples

- [ ] Fluid Simulator
  * [simFluid](https://github.com/mlhonke/simFluid) - C++ Fluid simulator, of interest: `src/sim_water.cpp` 
- Cellular Automata
 * [ ] **Navier-Stokes**.
      [This document](http://graphics.cs.cmu.edu/nsp/course/15-464/Fall09/papers/StamFluidforGames.pdf)
      looks promising.
 * [ ] [Cyclic Cellular Automaton](https://www.arnevogel.com/cyclic-cellular-automaton/) - That spiral thing of long ago.
 * [ ] [Simple Fluid Simulation](https://w-shadow.com/blog/2009/09/01/simple-fluid-simulation/)
 * [ ] [Lattice gas automaton](https://en.wikipedia.org/wiki/Lattice_gas_automaton)
- [ ] [GAN Models](https://machinelearningmastery.com/what-are-generative-adversarial-networks-gans/)
- [ ] Enhanced precision using [correction of rounding errors](http://andrewthall.org/papers/df64_qf128.pdf)
- [ ] Option for disabling L2 cache, for decent cooperation with `OpenGL`.
      **NOTE:** Perhaps needs  kernel built for L2 cache disabled. 
      **TODO** profile this!
- [ ] Make [ARCHITECTURE.md](https://matklad.github.io//2021/02/06/ARCHITECTURE.md.html) - [example](https://github.com/rust-analyzer/rust-analyzer/blob/master/docs/dev/architecture.md)
- [ ] Use [inherited enums](https://stackoverflow.com/questions/644629/base-enum-class-inheritance#644651) - for isolating DMA stuff
- [ ] Fourier Transform
  * [x] Implement DFT
  * [ ] Implement FFT - [O'Reilly](https://www.oreilly.com/library/view/c-cookbook/0596007612/ch11s18.html), [[https://scistatcalc.blogspot.com/2013/12/fft-calculator.html][Online FFT Calculator]], for comparison
  * [ ]  consider [sliding windows](https://github.com/glidernet/ogn-rf/issues/36#issuecomment-775688969)
- [ ] Etherium mining - [Proof of Work algorithm](https://github.com/chfast/ethash), [ethash spec revision 23](https://eth.wiki/en/concepts/ethash/ethash)
  * [ ] Keccak - derive from PoW project


