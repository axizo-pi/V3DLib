<head>
	<link rel="stylesheet" type="text/css" href="css/docs.css">
</head>

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

  This has an positive consequence: the new operations now return `Nan` and `Inf` as expected.
  The actual `SFU` functions return `0.0f` instead.


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

`load imm 32` loads a 4-byte value into a vector.
This is actually very useful and its presence is sorely missed on `v3d`.
On `vc4`, loading a constant value is a one-liner, on `v3d` you need to construct the constant with multiple operations.
I'm trying out alternatives, eg:

  - Loading constants as uniforms.
  - Initialize constants _once_ on kernel startup.

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
