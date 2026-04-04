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
- The user-level language is an [Embedded Domain Specific Language](https://wiki.c2.com/?EmbeddedDomainSpecificLanguage).
  There are no pre-processors being used other than the standard C pre-processor.
  The output is standard C++ code.
- Kernel programs are compiled dynamically, so that a given program can run unchanged on any version
  of the RaspBerry Pi.
  The kernels are generated inline and offloaded to the GPU's at runtime.


# Supported Pi's

I have about 14 Pi's foraging around here, which I intend to cluster.

Unit tests are run regularly on the following Pi versions:

| Pi   | Version         | 32/64bits | Debian distro | GPU | Model Number | Revision |
| ---- | --------------- | --------- | ------------- | --- | ------------ | -------- |
| Pi5  | Model B Rev 1.0 | 64        | 12 (bookworm) | vc7 | BCM2835      | c04170   |
| Pi4  | Model B Rev 1.1 | 64        | 12 (bookworm) | vc6 | BCM2711      | b03111   |
| Zero | W Rev 1.1       | 32        | 12 (bookworm) | vc4 | BCM2835      | 9000c1   |
| Pi3  | Model B Rev 1.2 | 32        | 10 (buster)   | vc4 | BCM2837      | a02082   |
| Pi2  | Model B Rev 1.1 | 32        | 12 (bookworm) | vc4 | BCM2836      | a01041   |
| Pi1  | Model B Rev 2   | 32        | 12 (bookworm) | vc4 | BCM2835      | 000e     |


The notable omissions in this list:

- **Raspberry PI Pico**: This is a microcontroller. Can't run Debian, has no VideoCore. 
- **Raspberry PI Compute Module 5**: As far as I can tell, this is a Pi5 in a sexy casing.
	I will buy it eventually, but currently I don't see the point.

# <a name="conventions">Conventions</a>

The following naming is used within the project:

- The `VideoCore IV` is referred to as `vc4`,
- The `VideoCore VI` as `vc6`,
- The `VideoCore VII` as `vc7`.
- `vc6` and `vc7` are collectively referred to as `v3d`[^1]. 

[^1]: This comes from the Mesa library. `vc6` and `vc7` are handled by a common driver called `v3d`.

- The earliest Debian version supported is **Debian 10 Buster**.
- 32-bits continues to be supported. This is required for the early PI's and `Zero`.
- The C++ code is currently compiled with language version `c++17`[^2]. 
- Indent is two spaces. Not because I want it to (I vastly prefer tabs), but because `github`
  otherwise makes a mess of the source display, especially when tabs and spaces are mixed.
- A program running on a VideoCore is named a [(compute) kernel](https://en.wikipedia.org/wiki/Compute_kernel).
	I leave out 'compute' when describing kernels.
  This is different from the Broadcomm and Mesa terminology, where programs are called **shaders**.
- A naming convention which has grown historically:
  * **scalar kernel** - A kernel written in standard C++, runs on CPU. 'scalar' for short.
  * **vector kernel** - A kernel written to run on the QPU's. 'kernel' for short.
- Values passed from a CPU program into a kernel are called *uniform values* or **uniforms**.
  Their usage is totally comparable to command-line parameters for a console app.
- Any method or function which generates debug output is named `dump()` (or uses `dump` as prefix).
  The utility of this is obvious when you use `dbg` a lot.

[^2]: There is no overriding reason to hold on to this, give me a good reason and I will happily up the C++ version.

-------------------

# Specifications 

Here is an overview for the easily comparable stuff:

| Pi Version | CPU # Cores |CPU Clock (Mhz) | GPU Clock (Mhz) | VideoCore version | Wifi | Ethernet |
|------------|-------------|----------------|-----------------|-------------------|------|----------|
| **Pi1**    | 1           |  700           | 250             | vc4               | no   | yes      |
| **Pi2**    | 4           |  900           | 250             | vc4               | no   | yes      |
| **Pi3**    | 4           | 1200           | 300             | vc4               | yes  | yes      |
| **Zero**   | 1           | 1000           | 300             | vc4               | yes  | **no**   |
| **Pi4**    | 4           | 1500           | 500             | vc6               | yes  | yes      |
| **Pi5**    | 4           | 2400           | 960             | vc7               | yes  | yes      |

The clock frequencies for both CPU and GPU are variable. The given values are the maximum values.

[Useful commands](Notes.html#useful-commands) for obtaining this information.

## GPU Stuff

| Item                 | vc4             | vc6              | vc7             | Comment |
|----------------------|-----------------|------------------|-----------------|---------|
| **Num QPU's:**       | 12              | 8                | 16              |         | 
| **Register Files**   | 2x32            | 1x64             | 1x64            |         |
| **Data Transfer**    |                 |                  |                 |         |
| DMA                  | read/write      | *not supported*  | *not supported* |         |
| VPM                  | read only       | read/write       | read/write      |         |
|                      |                 |                  |                 |         |
| _Following not verified for vc7_ |     |                  |                 |         |
| **TMU gather limit:**|  4              | 8                | | The maximum number of concurrent prefetches before QPU execution blocks |
| **Threads per QPU**  |                 |                  | | *Shows num available registers in register file per thread* |
| 1 thread             | 64 registers    |  *not supported* | |
| 2 threads            | 32 registers    | 64 registers     | |
| 4 threads            | *not supported* | 32 registers     | |

- Why is the TMU gather limit set here for `vc6`? DMA was dropped.

# Changes Between `VideoCore` versions

## Similarities between all versions 

- Every QPU has an add ALU and a multiply ALU which operate in parallel.
  A QPU can thus execute two ALU OP's per cycle.
- Every ALU has two input fields and one output field.
  These need not be all used, depending on the operation performed.
- There are 4 PPU's per ALU, which operate concurrently.
  A QPU can thus perform **8 operations per clock cycle**

## Significant Changes From `vc6` to `vc7`

- The accumulator registers have been removed. The input fields are now either
  a register address or a small immediate.
- Related to previous, all input fields can contain a distinct register address.
  This is different from `vc6`, where there can only be only _at most two distinct
  addresses_ over all input fields.

e.g.:

     add rf2, rf0, rf1 ; mul rf5, rf3, rf4     # Legal on vc7, invalid on vc6
     add rf2, rf0, rf0 ; mul rf5, rf0, rf1     # Legal on vc6

- Any input field can now contain a **small immediate**. On `vc6` this is limited to
  `raddr_b` (I realize this term might be unknown, but it is relevant).
  There still are limitations to the number of small immediates you can use in an
  operation, the limit still appears to be 1 at most.

There is an unfortunate consequence to this, as it appears that usage of small immediates
for `vc6` is actually more flexible. E.g.:

    shl rf0, 4, 4   # Legal on vc6, invalid on vc7

- The `SFU` (Special Functions Unit) has been dropped. All functions of the `SFU`
  are now done on the Add ALU.


## Significant Changes From `vc4` to `vc6`

- `vc6` has a single 64-register register file. `vc4` has two 32-register register files, A and B.
  You still only get an A read and a B read per instruction, but they read from one big register file.
- The VPM module, which handles DMA transfers, has been dropped. TMU is now read/write, on `vc4`
  this is read-only.
- v3d adds multi-gpu core support, each with their own set of QPUs.
  However, there is only one core in `vc6` (as well as `vc7`!).
- Special (hardware) registers are not mapped any more in the register file memory address.
  This means that you can _not do read operations on special registers_ any more in operations.
  Writes to special registers are still possible.
- Operations that read _and_ write to the register file now take 1 program cycle.
  On `vc4`, the operation stalls one cycle.

e.g.:

     add rf1, rf0, 1     # Two cycles on vc4, 1 on vc6

- Several instruction types have been dropped. `vc4` has the following distinct types:
  * `alu`
  * `alu small imm`
  * `branch`
  * `load imm 32`
  * `load imm per-elmt signed`
  * `load imm per-elmt unsigned`
  * `Semaphore`

Of these, only `alu` and `branch` remain on `vc6`.
The functionality of `alu small imm` has been merged into `alu`.

### Stuff I discovered during coding

#### - Float multiplication has been improved.

- On `vc4`, float multiplication on the QPU _always rounds downwards_
- On `vc6`, float multiplication rounds to the nearest value of the result

In other words, `vc6` will multiply as you would normally expect.
The result will be identical to float multiplication on the `ARM` processor.  
With `vc4` however, small differences creep in, which accumulate with continued computation.

#### - Integer multiplication improved

The following code yields different results for `vc4` and `vc6`

    Int a = 16;
    Int b = -1 * a;
    # vc4: b = 268435440
    # vc6: b = -16

This has to do with the integer multiply instruction working only on the lower 24 bits of integers.
As a consequence, a negative value gets its ones-complement prefix chopped off,
and whatever is left is treated as an integer.

#### - Setting of condition flags has changed
  * `vc4` - all conditions are set together, on usage condition to test is specified
  * `vc6` - a specific condition to set is specified, on usage a generic condition flag is read

To elaborate:

**vc4**: Each vector element has three associated condition flags:

- `N` - Negative
- `Z` - Zero
- `C` - Complement? By the looks of it `>= 0`, but you tell me. _(might be Carry)_

These are set with a single bitfield in an ALU instruction.
Each flag is explicitly tested in conditions.  
_See: "VideoCore IV Architecture Reference Guide", section "Condition Codes", p. 28._

**v3d**: Each vector element has two associated condition flags: `a` and `b`.

To set, a specific condition is specified in an instruction and the result is stored in `a`.
The previous value of `a` is put in `b`.  
_See: My brain after finally figuring this out._


## Further Changes

Only `vc4` has an architecture specification.
The stuff below is cobbled from whatever I and others have
[found out](https://www.raspberrypi.org/forums/viewtopic.php?t=244519).
The main strategy appears to be to investigate the available open source drivers.

It is perhaps necessary to note that there was also a 'VideoCore V' (let's call it `vc5`),
which was skipped in the Pi's.

- `vc5` has significant differences with `vc4`. `vc6` is an incremental change over `vc5`
- `vc5` added four threads per QPU mode, with 16 registers per thread.

### What remains the same

- Using threads in the QPU has effect upon the available resources: e.g. for two threads, the
  TMU depth is halved (to 4) and only half the registers in a register file are available.

Further:

- The instruction encoding for the QPUs is different, but the core instructions are the same.
- Instructions for packed 8 bit int math has been dropped, along with most of the pack modes.
- Instructions for packed 16 bit float math has been added (2 floats at in a single operation)
- the multiply ALU can now fadd, so you can issue two fadds per instruction.
- the add ALU has gained a bunch of new instructions.
- Most of the design changes have gone to improving the fixed function hardware around the QPUs.
- A fixed function blend unit has been added, which should reduce load on the QPUs when doing alpha blending.
- Some concern if software blending is still possible.
- The tile buffer can now store upto 4 render targets (I think it's up to 128bits per pixel, so if you are using 4 32bit render targets, you can't have a depth buffer)
- Faster LPDDR4 memory.
- A MMU, allowing a much simpler/faster kernel driver.
- Many more texture formats, framebuffer formats.
- All the features needed for opengl es 3.2 and vulkan 1.1
- With the threading improvements, the QPUs should spent much less time idle waiting for memory requests.

-----

# Compile times on all Pi's

To give you an idea of how long full compilation takes,
the following commands are used (20260207):

    > git pull
    > make clean
    > time make all runTest


Results per platform, time in seconds

| Platform | real (s) | user (s) |
|----------|----------|----------|
| Pi-1     | 6903     | 6036     |
| Zero     | 4288     | 4099     |
| Pi-2     | 1914     | 1798     |
| Pi-3     | 1011     |  886     |
| Pi-4     |  795     |  640     |
| Pi-5     |  130     |  119     |


-----

# Calculated theoretical max FLOPs

Let's be honest, the max FLOPs is totally unattainable in real life...

From the [VideoCore® IV 3D Architecture Reference Guide](https://docs.broadcom.com/doc/12358545):

- The QPU is a 16-way SIMD processor.
- Each processor has two vector floating-point ALUs which carry out multiply and non-multiply operations in parallel with single instruction cycle latency.
- Internally the QPU is a 4-way SIMD processor multiplexed 4× (over PPU's) over four cycles, making it particularly suited to processing streams of quads of pixels.

So:

- 4 operations per clock cycle, when properly pipelined
- 2 ALU's per operation, when instructions use both

So, calculation:

    op/clock per QPU = 4 [PPU's] x 2 [ALU's] = 8
    GFLOPs           = [Clock Speed (MHz)]x[num slices]x[qpu/slice]x[op/clock]
                     = [Clock Speed (MHz)]x[num qpu's]x[op/clock]
    
    Pi1  : 250x12x8 =  24.0 GFLOPs
    Pi2  : 250x12x8 =  24.0 GFLOPs
    Pi3  : 300x12x8 =  28.8 GFLOPs
    Zero : 300x12x8 =  28.8 GFLOPs
    Pi3+ : 400x12x8 =  38.4 GFLOPs  # Do I have one to verify?
    Pi4  : 500x 8x8 =  32.0 GFLOPs  # Less! The improved hardware in `v3d` compensates.
    Pi5  : 960x16x8 = 122.9 GFLOPS


-------------------

#### Footnotes
