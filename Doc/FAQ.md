<head>
	<link rel="stylesheet" type="text/css" href="css/docs.css">
</head>

Frequently Asked Questions
--------------------------

# Table of Contents

- [Differences in Execution](#differences-in-execution)
- [Calculated theoretical max FLOPs per QPU](#calculated-theoretical-max-flops-per-qpu)
- [Function `compile()` is not Thread-Safe](#not-thread-safe)
- [Handling privileges](#handling-privileges)
- [Issues with Old Distributions and Compilers](#issues-with-old-distributions-and-compilers)

-----

# Overview 

Here is an overview for the easily comparable stuff:

[Useful commands](#useful-commands) for obtaining this information.

| Pi Version | CPU # Cores |CPU Clock (Mhz) | GPU Clock (Mhz) | VideoCore version |
|------------|-------------|----------------|-----------------|-------------------|
| **Pi1**    | 1           |  700           | 250             | vc4               |
| **Pi2**    | 4           |  900           | 250             | vc4               |
| **Pi3**    | 4           | 1200           | 300             | vc4               |
| **Zero**   | 1           | 1000           | 300             | vc4               |
| **Pi4**    | 4           | 1500           | 500             | vc6               |
| **Pi5**    | 4           | 2400           | 960             | vc7               |


**GPU Stuff**

| Item                 | vc4             | vc6              | vc7             | Comment |
|----------------------|-----------------|------------------|-----------------|---------|
| **Num QPU's:**       | 12              | 8                | 16              |         | 
| **Register Files**   | 2x32            | 1x64             | 1x64            |         |
| **Data Transfer**    |                 |                  |                 |         |
| DMA                  | read/write      | *not supported*  | *not supported* |         |
| VPM                  | read only       | read/write       | read/write      |         |

Previous version:

| Item                 | vc4             | v3d              | Comment |
|----------------------|-----------------|------------------|-|
| **TMU gather limit:**|  4              | 8                | The maximum number of concurrent prefetches before QPU execution blocks |
| **Threads per QPU**  |                 |                  | *Shows num available registers in register file per thread* |
| 1 thread             | 64 registers    |  *not supported* | |
| 2 threads            | 32 registers    | 64 registers     | |
| 4 threads            | *not supported* | 32 registers     | |


# Changes Between `VideoCore` versions

## Similarities between all versions 

- Every QPU has an add ALU and a multiply ALU which operate in parallel.
  A QPU can thus execute two ALU OP's per cycle.
- Every ALU has two input fields and one output field.
  These need not be all used, depending on the operation performed.
- There 4 SIMD lanes, interleaved over 4 cycles.
  This means that a operation takes 4 clock cycles to complete, but execution
  of sequential operations overlaps.
  The result is that the throughput is still one operation per clock cycle.

## Significant Changes From `vc6` to `vc7`

- The accumulator registers have been removed. The input fields are now either
  a register address or a small immediate.
- Related to previous, all input fields can contain a distinct register address.
  This is different from `vc6`, where there can only be only _at most two distinct
  addresses_ over all input fields.

e.g.:

     add rf2, rf0, rf1 ; mul rf5, rf3, rf4     # Legal on vc7, invalid on vc6
     add rf2, rf0, rf0 ; mul rf5, rf0, rf1     # Legal on vc6

- The `SFU` (Special Functions Unit) has been dropped. All functions of the `SFU`
  are now done on the Add ALU.


## Significant Changes From `vc4` to `vc6`

- `vc6` has a single 64-register register file. `vc4` has two 32-register register files, A and B.
  You still only get an A read and a B read per instruction, but they read from one big register file.
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

Of those, only `alu` and `branch` remain on `vc6`.
The functionality of `alu small imm` has been merged into `alu`.

- Float multiplication has been improved.
	* On `vc4`, float multiplication on the QPU _always rounds downwards_
	* On `vc6`, float multiplication rounds to the nearest value of the result

In other words, `vc6` will multiply as you would normally expect.
The result will be identical to float multiplication on the `ARM` processor.  
With `vc4` however, small differences creep in, which accumulate with continued computation.

### Further Changes

Only `vc4` has an architecture specification.
The stuff below is cobbled from whatever I and others have found out.
The main strategy appears to be to investigate the available open source drivers.

[Source](https://www.raspberrypi.org/forums/viewtopic.php?t=244519)


It is perhaps necessary to note that there was also a 'VideoCore V' (let's call it `vc5`),
which was skipped in the Pi's.

- `vc5` has significant differences with `vc4`. `vc6` is an incremental change over `vc5`
- `vc5` added four threads per QPU mode, with 16 registers per thread.

### What remains the same

- The QPU pipeline stays mostly the same
- Using threads in the QPU has effect upon the available resources: e.g. for two threads, the
  TMU depth is halved (to 4) and only half the registers in a register file are available.

`V3DLib` **does not implement multi-threading** and never will.
The complexity is not worth it IMHO, and the benefits dubious.
I believe that there is no performance gain to be found here, quite the contrary. 

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

----

# Differences in Execution

This section records differences between the `vc4` and `v3d` QPU hardware and consequently in the instructions.

The `vc4`-specific items can be found in the "VideoCore IV Architecture Reference Guide";
the corresponding `v3d` stuff has mostly been found due to empirical research and hard thinking.

## Data Transfer

There are two transfer options, **VPM (DMA)** and **TMU**

- `vc4` has VPM for read/write and *read-only* TMU 
- `v3d` has *no* VPM[^1], and uses TMU for read/write

[^1]: *VPM IS mentioned in the QPU registers though, so it might be that I just never encountered*
      *its usage for `v3d`. This might be something I may investigate when bored and nothing else to do.*

**VPM**:

- can execute one read and on write in parallel,
- but multiple reads and multiple writes block each other.
- The QPU will stall if a read has to wait on a read, or a write has to wait on a write.
- It has the advantage of being able to handle multiple 16-vectors in one go.

**TMU**:

- does not block *and* operations can overlap, up to a limit.
- Up to 4 (`vc4`) or 8 (`v3d`) read operations can be performed together, and
- (apparently) an unlimited number of writes.
- The read/write can perform in parallel with the QPU execution.
- The QPU does not need to stall at all (but it *is* possible).
- However, only one 16-vector is handled per go.

On a rainy day, I did a [performance comparison between VPM and TMU](Profiling/ComparingVMPandTMU.html).
The conclusion is:

**For regular usage, TMU is always faster than VPM**

So it doesn't surpise me that VPM is dropped from `vc6` onward.


## Setting of condition flags

- `vc4` - all conditions are set together, on usage condition to test is specified
- `v3d` - a specific condition to set is specified, on usage a generic condition flag is read

To elaborate:

**vc4**

Each vector element has three associated condition flags:

- `N` - Negative
- `Z` - Zero
- `C` - Complement? By the looks of it `>= 0`, but you tell me

These are set with a single bitfield in an ALU instruction.
Each flag is explicitly tested in conditions.

See: "VideoCore IV Architecture Reference Guide", section "Condition Codes", p. 28.

**v3d**

- Each vector element has two associated condition flags: `a` and `b`

To set, a specific condition is specified in an instruction and the result is stored in `a`.
The previous value of `a` is put in `b`.

See: My brain after finally figuring this out.


## Integer multiplication

- `vc4`: multiplication of negative integers will produce unexpected results
- `v3d`: works as expected

The following source code statements yield different results for `vc4` and `v3d`

    a = 16
    b = -1 * a

- For `vc4`, the result is `268435440`
- For `v3d`, the result is `-16`

This has to do with the integer multiply instruction working only on the lower 24 bits of integers.
Thus, a negative value gets its ones-complement prefix chopped off, and whatever is left is treated as an integer.


-----

# Calculated theoretical max FLOPs per QPU

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

- Pi2  : 250x3x4x8 = 24.0 GFLOPs
- Pi3  : 300x3x4x8 = 28.8 GFLOPs
- Pi3+ : 400x3x4x8 = 38.4 GFLOPs
- Pi4  : 500x2x4x8 = 32.0 GFLOPs (less!)

- The improved hardware in `v3d` compensates for performance.
- v3d adds multi-gpu core support, each with their own set of QPUs. However, there is only one core in `v3d`.


-----

# <a name="not-thread-safe">Function `compile()` is not Thread-Safe</a>
Function `compile()` is used to compile a kernel from a class generator definition into a format that is runnable on a QPU. This uses *global* heaps internally for e.g. generating the AST and for storing the resulting statements.

Because the heaps are global, running `compile()` parallel on different threads will lead to problems. The result of the compile, however, should be fine, so it's possible to have multiple kernel instances on different threads.

As long a you run `compile()` on a single thread at a time, you're OK.


**TODO:** examine further.


-----

# Handling privileges

In order to use the `VideoCore`, special privileges are required to access certain devices files.  The default way is to run the applications with `sudo`.

You might run into the following situation (e.g.):

     > obj-qpu/bin/detectPlatform 
    Detected platform: Raspberry Pi 2 Model B Rev 1.1
    Can't open device file: /dev/vcio
    Try creating a device file with: sudo mknod /dev/vcio c 100 0

The solution for this is to become a member of group `video`:

     > sudo useradd -g video <user>


Where you fill in  a relevant user name for `<user>`. To enable this, logout and login, or start a new shell.

Unfortunately, this solution will not work for access to `/dev/mem`. You will still need to run with `sudo` for any application that uses the `VideoCore` hardware.


-------------------

# Issues with Old Distributions and Compilers

Following is known to occur with `Raspbian wheezy`.

* Certain expected functions are not defined

Following prototypes are missing in in `/opt/vc/include/bcm_host.h`:

  - `bcm_host_get_peripheral_address()`
  - `bcm_host_get_peripheral_size()`


-------------------

## <a name="useful-commands">Useful Commands</a>

Values here are for `Pi5`, unless otherwise specified.

**Get GPU Speed**

`Pi5`:

    > vcgencmd get_config int | grep v3d
    v3d_freq=960
    v3d_freq_min=500

`Pi4`, `Zero`:

    > vcgencmd get_config int | grep gpu
    gpu_freq=500
    ...

`Pi3`:

    > sudo vcgencmd get_config int | grep gpu
    gpu_freq=300

Previous doesn't work on `Pi2`. Following is a tentative to get the QPU speed anyway:

    > sudo vcgencmd measure_clock core
    frequency(1)=250000000

Also works for `Pi1`.



**Get GPU info**

`Pi5`, `Pi4`. Among other info, number of QPU's.

    > cat /sys/kernel/debug/dri/0/v3d_ident
    Revision:   7.1.7.0
    MMU:        yes
    TFU:        no
    MSO:        yes
    L3C:        no (0kb)
    Core 0:
      Revision:     7.1
      Slices:       4
      TMUs:         4
      QPUs:         16
      Semaphores:   0

**Get CPU speed**

`lscpu` returns other stuff which may be useful, in particular `Core(s) per cluster`.

    > lscpu | grep CPU.*MHz
    CPU(s) scaling MHz:                   62%
    CPU max MHz:                          2400.0000
    CPU min MHz:                          1500.0000

Following works on `Pi2`, should be OK on other Pi's:

    > vcgencmd get_config int | grep freq
    arm_freq=900
    arm_freq_min=600
    ...

-
