<head>
	<link rel="stylesheet" type="text/css" href="css/docs.css">
</head>

# Data Transfer - DMA(TMU) and VPM

## Overview

There are two transfer options:

- **VPM** - 'Vertex Pipeline Memory', handles DMA transfers
- **TMU** - 'Texture and Memory Lookup Unit'

- `vc4` has VPM for read/write and *read-only* TMU 
- `v3d` has *no* VPM[^1], and uses TMU for read/write

[^1]: *VPM IS mentioned in the QPU registers though, so it might be that I just never encountered*
      *its usage for `v3d`. This might be something I may investigate when bored and nothing else to do.*

### VPM

- can execute one read and on write in parallel,
- but multiple reads and multiple writes block each other.
- The QPU will stall if a read has to wait on a read, or a write has to wait on a write.
- It has the advantage of being able to handle multiple 16-vectors in one go.

**IMPORTANT: The `DMA` sidesteps the L2 cache**

The L2 cache is not refreshed after a `DMA` write.
This is an issue when `VPM` and `TMU` are used on the same buffer object with a kernel execution.

### TMU

- Uses a single L2 cache for all slices.
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




----

# DMA and VPM (`vc4` only)

*There's a shitload of complexity here, most of which I don't want to deal with.
This section documents the stuff I need to know now ('now' being a moving target).*

See the example program `DMA` for basic usage, which is a level deeper than the regular use in the
source language. Normally, you won't explicitly use DMA and VPM at all.

`DMA` is just what you would expect, given the acronym.

The `VPM` (Vertex Pipe Memory) is a 12KB storage buffer, used to load and save data processed by the QPUs.
This buffer is shared by all QPUs, so there's plenty of opportunity for screwups in accessing it in a 
multi-QPU situation.

There are several memory mappings possible for the VPM, use of which is pretty arcane
(see the `VC IV Architecture reference Guide`).
I don't want to delve into this and just stick to the way `V3DLib` uses the VPM (which has been
inherited from `QPULib`). In particular, `V3DLib` restricts itself to *horizontal* memory accesses.

Loading values from shared main memory to QPUs is a two-stage process:

1. Start a DMA load to VPM
2. Load values from VPM into QPU registers

Saving values from QPUs to shared main memory is the process in reverse.

## DMA Usage

DMA deals with *bytes*.

- DMA read/write can operate in parallel with QPU execution. You will need to juggle a bit for
  optimal performance.
- Per QPU, a single DMA read and a single DMA write can execute in parallel.
- Multiple reads need to be processed sequentially. Either you wait for the previous read to finish,
  of the subsequent read blocks until the current read is finished.
- Likewise for multiple writes.


### Load example in source language (taken from example `DMA`)

The DMA load is first configured, then you start it and wait for it to complete.

To load 16 consecutive 16-byte vectors from shared main memory to (byte) address 0 in the VPM:

    dmaSetReadPitch(64);               // 64: size of one 16-byte vector
    dmaSetupRead(HORIZ, 16, 0);        // 16: number of vectors to load; 0: target address in VPM
    
    dmaStartRead(p);                   // p:  source pointer in shared memory
    dmaWaitRead();                     // Wait until load complete

C++ pseudo code:

     byte *p         = <assigned value>;
     byte *vpm       = 0;
     int pitch       = 64;
     int num_vectors = 16;

     for (int i = 0; i < num_vectors; i++) {
       for (int j = 0; j < pitch; j++) {
         *(vpm + j) = *(p + j);
       }
       vpm += pitch;
       p   += pitch;
     }



### Store example in source language (adapted from example `DMA`)

The DMA store is first configured, then you start it and wait for it to complete.

This example is a bit more elaborate, to better illustrate how the stride works.
For full vector transfers, the call to `dmaSetWriteStride()` is removed, as well as 
the last parameter to `dmaSetupWrite()` (see example program `DMA`).

To move **the first 3 values** of 16 consecutive 16-byte vectors
from (byte) address 256 in VPM to shared main memory:

    dmaSetWriteStride(13*4);           // Skip 13 values of vector
    dmaSetupWrite(HORIZ, 16, 256, 3);  // 16:  number of vectors to handle;
                                       // 256: start address for read;
                                       // 3:   number of consecutive values to transfer
    dmaStartWrite(p);                  // p:   target address in shared main memory
    dmaWaitWrite();                    // Wait until store complete

Note the discrepancy in parameter types:
- in `dmaSetWriteStride()`, the parameter is in **bytes**
- in `dmaSetupWrite()`, the final parameter is in **words**, i.e. the number of 4-byte elements

This is a source language thing which could be changed, but I do not want to go there.
I just want to understand it.

C++ pseudo code:

    // Assume that word size is 4 bytes
    
    byte *p         = <assigned value>;
    byte *vpm       = 256;
    int num_elems   = 3;
    int stride      = 13*4;  // i.e. 16 - num_bytes
    int num_vectors = 16;
    
    for (int i = 0; i < num_vectors; i++) {
      for (int j = 0; j < num_elems; j++) {
        *((word *) p) = *((word *) vpm);
        vpm += sizeof(word);
        p   += sizeof(word);
      }
      vpm += stride;
      p   += stride;
    }


## VPM Usage

VPM deals with 64-byte *vectors*.

Sizes and VPM addresses are defined in terms of these vectors, i.e. a size of '1' means one 64-byte vector,
and and address of '1' means the VPM data location of the second vector in a sequence.

VPM load/store is configured initially; this sets up offset values which are updated on every access.

The example `DMA` combines the load and store in an artful manner.

### Example of loading values to QPU:

Load 16 consecutive values from VPM into a QPU register:

    Int a;                             // Variable which is assigned to a QPU register on compile
    vpmSetupRead(HORIZ, 16, 0);        // 16: number of vectors to load; 0: start index of vectors
    
    for (int i = 0; i < 16; i++) {     // Read each vector
      a = vpmGetInt();
      // Do some operation on value here
    }

C++ pseudo code:

    vector *vpm_in  = 0;   // vector: 64-byte data structure; 0: starting index of vectors
    int num_vectors = 16;
    
    for (int i = 0; i < num_vectors; i++) {
      a = *vpm_in;
      vpm_in++;
      // Do some operation on value here
    }


### Example of saving values from QPU:

    vpmSetupWrite(HORIZ, 16);          // 16: vector index to store at
    
    Int a = 0;
    for (int i = 0; i < 16; i++) {     // write back values of a
      vpmPut(a);
      a++;
    }

C++ pseudo code:

    vector *vpm_out = 16;                // vector: 64-byte data structure; 16: starting index of vectors
    int num_vectors = 16;
    
    Int a = 0;
    for (int i = 0; i < num_vectors; i++) {
      *vpm_out = a;
      vpm_out++;
      a += 1;
    }


-----

# Things to Remember

*Just plain skip this if you're browsing. It's only useful for archaeological purposes. Must rid myself of this hoarding tendency one day.*

## vc4 DMA write: destination pointer is impervious to offset changes

For `vc4`, when doing DMA writes, the index offset is taken into account
in the DMA setup, therefore there is no need to add it.
In fact, the added offset is completely disregarded.

This came to light in a previous version of the `DSL` unit test.
The following was done before a write (kernel source code):

    outIndex = index();
    ...
    result[outIndex] = res;
    outIndex = outIndex + 16;

I would expect DMA to write to wrong locations, **but it doesn't**.
The DMA write ignores this offset and writes to the correct location, i.e. just like:

    ...
    *result = res;
    ...

My working hypothesis is that only the pointer value for vector index 0 is used to
initialize DMA.


## Initialization of stride for `vc4` at the level of the target language

This places the initialization code in the INIT-block, after translation of source to target.
The INIT-block therefore needs to be added first.

The pointer offset is only effective for TMU accesses.
When VPM/DMA is used, the index number is compensated for automatically, hence no need for it.

    void SourceTranslate::add_init(Seq<Instr> &code) {
    	using namespace V3DLib::Target::instr;

    	int insert_index = get_init_begin_marker(code);

    	Seq<Instr> ret;

    /*
      // Previous version, adding an offset for multiple QPUs
      // This was silly idea and has been removed. Kept here for reference
      // offset = 4 * (vector_id + 16 * qpu_num);
      ret << shl(ACC1, rf(RSV_QPU_ID), 4) // Avoid ACC0 here, it's used for getting QPU_ID and ELEM_ID (next stmt)
          << mov(ACC0, ELEM_ID)
          << add(ACC1, ACC1, ACC0)
          << shl(ACC0, ACC1, 2)           // offset now in ACC0
          << add_uniform_pointer_offset(code);
    */

      // offset = 4 * vector_id;
      ret << mov(ACC0, ELEM_ID)
          << shl(ACC0, ACC0, 2)             // offset now in ACC0
          << add_uniform_pointer_offset(code);
    
    	 code.insert(insert_index + 1, ret);  // Insert init code after the INIT_BEGIN marker
    }
