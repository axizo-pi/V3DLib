<head>
	<link rel="stylesheet" type="text/css" href="css/docs.css">
</head>

# Useful Links

## RNN

* Considering `tanh()` because it makes for a good sigmoid funtion.
  - [tanh numeric approximation] - tricky to vectorize due to if-blocks
  - [Approximation using exp()](https://stackoverflow.com/q/29239343/12235310) - Wikipedia
  - Taylor approximation: [Wikipedia hyperbolic functions], scroll to **Taylor series expressions**
* Tutorial [RNN on MNIST](https://github.com/dmlc/minpy/blob/master/examples/tutorials/rnn_mnist.ipynb)
* [mnist-from-scratch](https://github.com/CermakM/mnist-from-scratch/tree/master) - implemntation in C++
* [LSTM](https://en.wikipedia.org/wiki/Long_short-term_memory) - 'Long Short-Term Memory'; alternative to RNN

## VideoCore IV 

* The [VideoCore IV Reference Manual] by Broadcom. [Errata].
* The [documentation, demos, and assembler](https://github.com/hermanhermitage/videocoreiv-qpu)
  by Herman Hermitage.
* The [FFT implementation](http://www.aholme.co.uk/GPU_FFT/Main.htm)
  by Andrew Holme. [Blog](https://www.raspberrypi.org/blog/accelerating-fourier-transforms-using-the-gpu/)

## VideoCore VI 

* [v3d driver code in the linux kernel repository] - [v3d in kernel on github].
  Of special interest: [v3d_gem.c], [v3d_drm.h]. `vc4` code is  on same level
* [MESA v3d driver] - [github], `vc4` on same level
* [py-videocore6](https://github.com/Idein/py-videocore6) - Python project hacking the `VideoCore VI`
* [Broadcom code for v3d] - [relevant part], not sure if this is also for `vc4`, 2010 so probably no
- [Source doc for registers] - contains registers not in the ref doc:
- [Broadcom VideoCore V QPU Instruction Set] - [translation]
- [Notowaga example code] - useful!
- [Linux doc for v3d] - this is vc6


## Tools

* [Online STL Viewer](https://www.viewstl.com/#!)
* [vcgencmd](https://www.raspberrypi.org/documentation/raspbian/applications/vcgencmd.md)
* [doctest](https://github.com/onqtam/doctest) - unit test framework, [Documentation links](https://github.com/onqtam/doctest#documentation)

## Other

* [N-body simulation](https://en.wikipedia.org/wiki/N-body_simulation) - Wikipedia

--------------------------

[Wikipedia hyperbolic functions]: https://en.wikipedia.org/wiki/Hyperbolic_functions
[tanh numeric approximation]: https://github.com/rrottmann/anguita/blob/0e6247694d09f1331389d720e39301c94df831d4/anguita/__init__.py#L9
[VideoCore IV Reference Manual]: https://docs.broadcom.com/docs-and-downloads/docs/support/videocore/VideoCoreIV-AG100-R.pdf
[Errata]: https://www.elinux.org/VideoCore_IV_3D_Architecture_Reference_Guide_errata
[v3d driver code in the linux kernel repository]: https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tree/drivers/gpu/drm/v3d
[v3d in kernel on github]: https://github.com/torvalds/linux/tree/master/drivers/gpu/drm/v3d
[v3d_gem.c]: https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tree/drivers/gpu/drm/v3d/v3d_gem.c
[v3d_drm.h]: https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tree/include/uapi/drm/v3d_drm.h
[MESA v3d driver]: https://gitlab.freedesktop.org/mesa/mesa/-/tree/master/src/gallium/drivers/v3d
[github]: https://github.com/intel/external-mesa/tree/master/src/gallium/drivers/v3d
[Broadcom code for v3d]: https://android.googlesource.com/kernel/bcm/+/android-bcm-tetra-3.10-kitkat-wear/drivers/char/broadcom/mm/v3d/
[relevant part]: https://android.googlesource.com/kernel/bcm/+/android-bcm-tetra-3.10-kitkat-wear/drivers/char/broadcom/mm/v3d/v3d_user.c#179
[Source doc for registers]: https://vc4-notes.tumblr.com/post/125039428234/v3d-registers-not-on-videocore-iv-3d-architecture]
[Broadcom VideoCore V QPU Instruction Set]: http://imrc.noip.me/blog/vc4/VC5_instruction_set/
[translation]: https://translate.google.com/translate?hl=en&sl=auto&tl=en&u=http%3A%2F%2Fimrc.noip.me%2Fblog%2Fvc4%2FVC5_instruction_set%2F
[Notowaga example code]: https://gist.github.com/notogawa/36d0cc9168ae3236902729f26064281d
[Linux doc for v3d]: https://dri.freedesktop.org/docs/drm/gpu/v3d.html
