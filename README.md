<head>
  <link rel="stylesheet" type="text/css" href="Doc/css/docs.css">
</head>

# V3DLib

**Version 0.8.0**


`V3DLib` is a C++ library for creating programs to run on the VideoCore GPU's of all versions of the [Raspberry Pi](https://www.raspberrypi.org/).

## Recent History

**20260404 - I just got this project on GitHub. Lots of things broken, busy fixing it**

**20260407** - All units tests passing on all Pi's. There are still issues for non-QPU mode compilation[^1].

**20260411** - Units tests passing for non-QPU mode on Pi's. Still issues for non-Pi.

**20260416** - Compiling for Debian 13 (Trixie) on Pi. Works in principle, but need to fix issues.
               **Fixed**. All unit tests passing for this case.

[^1]: **Non-QPU mode** is intended to make the project compilable for platforms without a `VideoCore`.
      Seriously considering dropping the non-QPU mode, I don't see the point any more.
      Previously, it had some use to compile on my Intel i7 because it was faster; nowadays, a `Pi5` is plenty fast enough.


## Supported Pi's

The GPU on the Pi's is called the **VideoCore**.
There are three different version of the VideoCore over all Pi versions.

The Following Pi's are supported:

| Pi   | VideoCore Version | 
| ---- | ----------------- |
| Pi5  | VII               |
| Pi4  | VI                |
| Zero | IV                |
| Pi3  | IV                |
| Pi2  | IV                |
| Pi1  | IV                |

## Motivation

The Raspberry Pi's have a pretty nifty *general-purpose* GPU, the `VideoCore`.
It bothered me to no end that this GPU largely unused; the only thing really using it is `OpenGL`.

The Pi5 has a new version of the GPU. I resolved to update the project when I got the Pi5 to pass all unit tests;
I have reached this point.

I lost my previous account due to 'enhancements' in the GitHub authentication.
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
