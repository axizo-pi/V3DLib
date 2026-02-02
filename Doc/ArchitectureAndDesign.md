<head>
	<link rel="stylesheet" type="text/css" href="css/docs.css">
</head>

# Architecture and Design

This document explains the basics of the QPU architecture and documents some design decisions within `V3DLib` to deal with it.

-----

# VideoCore Overview

There are three distinct versions of the `VideoCore` GPU:

- **VideoCore IV**  - Used on all Pi's prior to the `Pi4`, and on the `Zero` 
- **VideoCore VI**  - Used on The `Pi4`
- **VideoCore VII** - Used on The `Pi5`

The VideoCore is a [vector processor](https://en.wikipedia.org/wiki/Vector_processor)
developed by [Broadcom](http://www.broadcom.com/) with
instructions that operate on 16-element vectors of 32-bit integer or floating point values.  
In other words. the VideCore is an **SIMD processor** ('Single Instruction, Multiple Data').

The basic hardware unit in the VideoCore is the **Quad Processing Unit (QPU)**.

- The `Pi4` has 8 QPU's 
- The `Pi5` has 16
- All other Pi's have 12

Due to the hardware improvements, the `Pi4` GPU is still faster than in previous versions of the Pi.  
Performance-wise, the `Pi5` absolutely destroys all previous models.

To get an idea of what VideoCore programming looks like, please view the [Basics Page](Basics.md).

## Personal Notes/Gripes

- The QPUs are part of the Raspberry Pi's graphics pipeline.  If you're
interested in doing cool graphics on the Pi then you probably want **OpenGL ES**.
The added value of `V3DLib` is _accelerating non-graphics parts_ of your Pi projects.
- `V3DLib` **does not implement multi-threading** on the QPU level and never will.
The complexity is not worth it IMHO, and the benefits dubious.
I believe that there is no performance gain to be found here, quite the contrary. 
- `V3DLib` **is not thread-safe**.
  In particular, function `compile()` is used to compile a kernel from a class generator definition into
  a format that is runnable on a QPU. This uses *global* heaps internally for e.g. generating
  the AST and for storing the resulting statements. Because the heaps are global, running
 `compile()` parallel on different threads will lead to problems.
- The user-level language is an [Embedded Domain Specific Language](https://wiki.c2.com/?EmbeddedDomainSpecificLanguage){:target="_blank"}.
  There are no pre-processors being used other than the standard C pre-processor.
  The output is standard C++ code.


# Supported Pi's

I have about 14 Pi's foraging around here, which I intend to cluster.

Unit tests are run regularly on the following Pi versions:

| Pi   | Version         | 32/64bits | Debian distro |
| ---- | --------------- | --------- | ------------- |
| Pi5  | Model B Rev 1.0 | 64        | 12 (bookworm) |
| Pi4  | Model B Rev 1.1 | 64        | 12 (bookworm) |
| Zero | W Rev 1.1       | 32        | 12 (bookworm) |
| Pi3  | Model B Rev 1.2 | 32        | 10 (buster)   |
| Pi2  | Model B Rev 1.1 | 32        | 12 (bookworm) |
| Pi1  | Model B Rev 2   | 32        | 12 (bookworm) |

The notable omissions in this list:

- **Raspberry PI Pico**: This is a microcontroller. Can't run Debian, has no VideoCore. 
- **Raspberry PI Compute Module 5**: As far as I can tell, this is a Pi5 in a sexy casing.
	I will buy it eventually, but currently I don't see the point.

# Conventions

The following naming is used within the project:

- The `VideoCore IV` is referred to as `vc4`,
- The `VideoCore VI` as `vc6`,
- The `VideoCore VII` as `vc7`.
- `vc6` and `vc7` are collectively referred to as `v3d`[^1]. 

[^1]: This comes from the Mesa library. `vc6` and `vc7` are handled by a common driver called `v3d`.

- The earliest Debian version supported is **Debian 10 Buster**.
- 32-bits continues to be supported. This is required for the early PI's and `Zero`.
- The C++ code is currently compiled with language version `c++17`. There is no overriding reason
  to hold on to this, give me a good reason and I will happily up the C++ version.
- Indent is two spaces. Not because I want it to (I vastly prefer tabs), but because `github`
  otherwise makes a mess of the source display, especially when tabs and spaces are mixed.
- A program running on a VideoCore is named a [(compute) kernel](https://en.wikipedia.org/wiki/Compute_kernel).
	I leave out 'compute' when describing kernels.
  This is different from the Broadcomm and Mesa terminology, where programs are called **shaders**.
- A naming convention which has grown historically:
  * **scalar kernel** - A kernel written in standard C++, runs on CPU. 'scalar' for short
  * **vector kernel** - A kernel written to run on the QPU's. 'vector' for short
- Values passed from a CPU program into a kernel are called *uniform values* or **uniforms**.
- Any method or function which generates debug output is named `dump()` (or uses `dump` as prefix).
  The utility of this is apparent when you run `dbg` a lot.

Kernel programs are compiled dynamically, so that a given program can run unchanged on any version
of the RaspBerry Pi.
The kernels are generated inline and offloaded to the GPU's at runtime.



# QPU Registers

## Vector Offsets

In a kernel, when loading values in a register in a manner that would be considered intuitive for a programmer:

    Int a = 2;

...you end up with a 16-vector containing the same values:

    a = <2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2>

In order to use the vector processing capabalities effectively, you want to be able to perform the calculations
with different values.

The following functions at source code level are supplied to deal this:

### Function `index()`

Returns an index value unique to each vector element, in the range `0..15`.

The following user-level code:

    Int a = index();

Results in: 

    a = <0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15>

### Function `me()`

Returns an index value unique to each QPU participating in a calculation.
A single running QPU would have `me() == 0`, any further QPU's are indexed sequentially.

### Function `numQPUs()` 

Returns  number of QPU's participating in a calculation.

The possible values depend on the VideoCore used:

- `vc4`: 1...12
- `vc6`: 1 or 8
- `vc7`: 1...16

### Vector offset calculation

The previous functions are useful to differentiate pointers to memory addresses.
The following is a method to load in consective values from shared main memory:

    void kernel(Ptr<Int> x) {
      x = x + index();
      a = *x;
    }

*Keep in mind that Int and Float values are 4 bytes. Pointer arithmetic takes this into account.*

The incoming value `x` is a pointer to an address in shared memory (i.e. accessible by both the CPU and the QPU's).
By adding `index()`, each vector element of `x` will point to consecutive values.
On the assignment to `a`, these consecutive values will be loaded into the vector elements of `a`.


When using multiple QPUs, you could load consecutive blocks of values into separate QPUs int the following way:

    void kernel(Ptr<Float> x) {
      x = x + index() + (me() << 4);
      a = *x;
    }


## Automatic Uniform Pointer Initialization

Adding `index()` to uniform pointers is so common in kernel code, that I made the following design decision:

**All uniform pointers are initialized with an index offset**

This means that if a parameter `Int::Ptr ptr` is passed into a kernel with an assigned memory address value `addr`,
It will be initialized as:

    ptr = <addr addr+4 addr+8 addr+12 addr+16 addr+20 addr+24 addr+28 addr+32 addr+36 addr+40 addr+44 addr+48 addr+52 addr+56 addr+60>

You are free to adjust the offsets as required in your application.
A nice example is **Cursors**.

If you really need a single address in the code, do the following in the kernel:

    ptr -= index();

-----

# Setting of Branch Conditions

**TODO:** Make this a coherent text.

Source: qpu_instr.h, line 74, enum v3d_qpu_uf:

How I interpret this:

- AND: if all bits set
- NOR: if no bits set
- N  : field not set
- Z  : Field zero
- N  : field negative set
- C  : Field negative cleared

What the bits are is not clear at this point.
These assumptions are probably wrong, but I need a starting point.

**TODO:** make tests to verify these assumptions (how? No clue right now)

So:

- vc4 `if all(nc)...` -> ANDC
- vc4 `nc` - negative clear, ie. >= 0
- vc4 `ns` - negative set,   ie.  < 0

-----

#### Footnotes
