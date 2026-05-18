<head>
  <link rel="stylesheet" type="text/css" href="Doc/css/docs.css">
</head>

# V3DLib

**Version 0.8.1**

`V3DLib` is a C++ library for programming the GPU's of _all_ versions of the [Raspberry Pi](https://www.raspberrypi.org/).

The GPU on the Pi's is called the `VideoCore`.
The `VideoCore` is an **SIMD[^1] [vector processor](https://en.wikipedia.org/wiki/Vector_processor)**
developed by [Broadcom](http://www.broadcom.com/).
It employs instructions that operate on 16 consecutive values of 32-bit integer or floating point values.  

[^1]: **SIMD**: Single Instruction, Multiple Data

`V3DLib` offers a high-level programming language to facilitate `VideoCore` programming.  
There are currently three different version of the VideoCore.
All (three) versions of `VideoCore` are supported.

_If you want to do fancy direct 3d rendering, `OpenGL` is a better choice. This library is intended
to make **general purpose** calculations available on the Pi GPU._


## Example

At the risk of insulting your intelligence, following is the kernel of example program `Hello`:

    void hello(Int::Ptr p) {
      *p = 1;
    }

This code translates directly to QPU instructions. The thing to take away here is:

**All values in `QPU` are 16 4-byte values.**

- A parameter of type `Int` is actually represents 16 integer values.
- An `Int::Ptr` thus points to 16 consecutive integer values. The single operation in the kernel
  sets _all_ of these integers to `1`.

To get to grip on `VideoCore` programming, see the [Basics Page](Doc/Basics.md).

## Motivation

- The Raspberry Pi's have a pretty nifty *general-purpose* GPU, the `VideoCore`.
  It bothered me to no end that this GPU largely unused; the only thing really using it is `OpenGL`.
- The Pi5 has a new version of the GPU. I resolved to update the project when I got the Pi5 to pass all unit tests;
  I have reached this point.
- I lost my previous account due to changes in the GitHub authentication.
  This account replaces [my previous account](https://github.com/wimrijnders).

## Credit where Credit is Due

This project builds upon the [QPULib](https://github.com/mn416/QPULib) project, by **Matthew Naylor**.
I fully acknowledge his work for the `VideoCore IV` and am grateful for what he has achieved.
Plenty of his original code is still present in this project.  
`QPULib`, however, is no longer under development, and I felt the necessity to expand upon it.


## Getting Started

- For a global view of this project and the `VideoCore`, see the [Overview Page](Doc/Overview.md).
- If you are interested in performance, view the [Mandelbrot Profiling Page](Doc/Profiling/mandelbrot.md).  
  _Purchasing tip: Go straight to `Pi5`, don't bother with the rest._
- Scan the example programs to get an idea of the high-level programming language.
  Start with program `Examples/Hello`, perhaps the simplest possible program.
- To get a grip of what VideoCore programming looks like, view the [Basics Page](Doc/Basics.md).
- To install a PI with `V3DLib`, see the [Install Instructions](Doc/install.md).
- If you want to help in development, you are more than welcome. I warn you that it will be an uphill battle
  * Scan the [conventions](Doc/Overview.md#conventions). Read the rest of the page at your own leisure.
  * Please look at the [Known Issues](Doc/Issues.md), so you have an idea what to expect.

--------------------------

#### Footnotes
