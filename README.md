<head>
	<link rel="stylesheet" type="text/css" href="Doc/css/docs.css">
</head>

# V3DLib

**Version 0.8.0**

`V3DLib` is a C++ library for creating programs to run on the VideoCore GPU's of all versions of the [Raspberry Pi](https://www.raspberrypi.org/){:target="_blank"}.

The Raspberry Pi's have a pretty nifty *general-purpose* GPU, the `VideoCore`.
It bothered me to no end that this GPU largely unused; the only thing really using it is `OpenGL`.

The Pi5 has a new version of the GPU. I resolved to update the project when I got the Pi5 to pass all unit tests[^1];
I have reached this point.

I lost my previous account due to 'enhancements' in the GitHub authentication.
The current account replaces [my previous account](https://github.com/wimrijnders){:target="_blank"}.

## Supported Pi's

The Following Pi's are supported:

| Pi   | VideoCore Version | 
| ---- | ----------------- |
| Pi5  | VII               |
| Pi4  | VI                |
| Zero | IV                |
| Pi3  | IV                |
| Pi2  | IV                |
| Pi1  | IV                |

The notable omissions in this list:

- **Raspberry PI Pico**: This is a microcontroller. Can't run Debian, has no VideoCore. 
- **Raspberry PI Compute Module 5**: As far as I can tell, this is a Pi5 in a sexy casing.
	I will buy it eventually, but currently I don't see the point.


## Credit where Credit is Due

This project builds upon the [QPULib](https://github.com/mn416/QPULib) project, by **Matthew Naylor**.
I fully acknowledge his work for the `VideoCore IV` and am grateful for what he has achieved.
Plenty of his original code is still present in this project.  
`QPULib`, however, is no longer under development, and I felt the necessity to expand upon it.


## Getting Started

- For a global view of this project and the `VideoCore`, see the [Overview Page](Doc/Overview.md).
- If you are interested in performance, view the [Mandelbrot Profiling Page](Profiling/mandelbrot.md).  
  _Purchasing tip: Go straight to `Pi5`, don't bother with the rest._
- Scan the example programs to get an idea of the high-level programming language.
  Start with program `Examples/Hello`, perhaps the simplest possible program.
- To get a grip of what VideoCore programming looks like, view the [Basics Page](Basics.md).
- To install a PI with `V3DLib`, see the [Install Instructions](Doc/install.md).
- If you want to help in development, you are more than welcome. I warn you that it will be an uphill battle
  * Scan the coding conventions at the top of the [Architecture and Design Page](Doc/ArchitectureAndDesign.md). Read the rest at your own leisure.
  * Please look at the [Known Issues](Doc/BuildInstructions.md#known-issues), so you have an idea what to expect.

--------------------------

[^1]: **TODO** Fill this in. I lost the previous text.

