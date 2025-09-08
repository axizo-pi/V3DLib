
- Set locale to `en_US.UTF-8' using `sudo raspi-config`, Localization Options.

- For external libs:

    > sudo apt-get install cmake

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
