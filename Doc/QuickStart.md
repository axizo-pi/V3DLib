
- Set locale to `en_US.UTF-8' using `sudo raspi-config`, Localization Options.

- For external libs:

    > sudo apt-get install cmake

- For new mesa(2) lib:

    > sudo apt-get install libdrm-dev

---------------------------

On Debian 12 Woodworm, Pi <= 3:

**NOT WORKING - Mandelbrot still can't access /dev/mem**

> sudo usermod -a -G kmem [your-username]		# For accessing /dev/mem without sudo

---------------------------

### Memory Split

On Debian 12 Woodworm, it is no longer possible to set the memory split using `raspi-config`.
The default memory split appears to be 76MB for graphic, which normally should be plenty.

- View memmory split:

> vcgencmd get_mem gpu 		# Display current memory split

- Change memory split:

> sudo vi /boot/firmware/config.txt

Add at end of file:

> gpu_mem=16

Save and quit, then reboot:

> sudo reboot

---------------------------
### Links

- [Raspberry Pi repo](https://github.com/raspberrypi/) - base
Juicy bits:
    - [vcsm userland](https://github.com/raspberrypi/userland/blob/master/host_applications/linux/libs/sm/)
    - [broadcom](https://github.com/raspberrypi/linux/tree/rpi-6.12.y/include/linux/broadcom)
- [rpivcsm](https://github.com/Idein/rpi-vcsm/tree/master) - python project to access vcsm
- [linux kernel vc4](https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tree/drivers/gpu/drm/vc4)
- [Mesa Broadcom repo](https://gitlab.freedesktop.org/mesa/mesa/-/tree/main/src/broadcom?ref_type=heads)

