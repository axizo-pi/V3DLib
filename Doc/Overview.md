<head>
	<link rel="stylesheet" type="text/css" href="css/docs.css">
</head>

# Overview

## VideoCore Basics

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

### Personal Notes/Gripes

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

----

#### Footnotes
