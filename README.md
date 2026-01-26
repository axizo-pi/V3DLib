# V3DLib

**Version 0.8.0**

`V3DLib` is a C++ library for creating programs to run on the VideoCore GPU's of all versions of the [Raspberry Pi](https://www.raspberrypi.org/){:target="_blank"}.

The Pi5 has a new version of the GPU. I resolved to update the project when I got the Pi5 to pass all unit tests[^1]; I reached this point.

In addition, I lost my previous account due to 'enhanced restriction' of the GitHub authentication. The current account replaces [my previous account](https://github.com/wimrijnders){:target="_blank"}.

[^1]: This is not entirely true; there is *one* unit test which can not be run within the context of all unit tests. However, it runs fine when run on its own.


## Motivation for this Project

The Raspberry Pi's have a pretty nifty *general-purpose* GPU, the `VideoCore`.
It bothered me to no end that this GPU largely unused; the only thing really using it is `OpenGL`.

The goal of this project is to make **the GPU accessible** for programming, and
to **ease the pain** of using it.

`V3DLib` contains a high-level programming language for simplifying[^3] GPU-programming.
See program `Examples/Hello` for the simplest possible example program.

[^3]: I hope.

## Getting Started

To install a PI with `V3DLib`, see the [Install Instructions](Doc/install.md).

The VideCore is an **SIMD processor**.
If the term `SIMD`[^2] is new to you, please look at the [Basics Page](Doc/Basics.md), as it applies
to this project.

[^2]: SIMD: Single Instruction, Multiple Data. The VideoCore does operations on 16 values in one go.


===== ** Till here ** =====

- For starters, scan the naming conventions at the top of the [Architecture and Design Page](Doc/ArchitectureAndDesign.md). Read the rest at your own leisure.
- Please look at the [Known Issues](Doc/BuildInstructions.md#known-issues), so you have an idea what to expect.


## Credit where Credit is Due

This project builds upon the [QPULib](https://github.com/mn416/QPULib) project, by **Matthew Naylor**.
I fully acknowledge his work for the `VideoCore IV` and am grateful for what he has achieved in setting
up the compilation and assembly.

`QPULib`, however, is no longer under development, and I felt the need to expand it to support
the `VideoCore VI` as well. Hence, `V3DLib` was conceived.

--------------------------

