<head>
	<link rel="stylesheet" type="text/css" href="css/docs.css">
</head>

# Install Instructions

I'm sorry, I can't make this easy. You've got to jump through several hoops to make it work.

It is not possible to run the library on Pi's which have graphical output[^1]. The Pi running
the library needs to be a *headless platform*, not using any graphical capabilities.

[^1]: Not entirely true. The library will work *sometimes* if you're lucky. The only Pi on which
			successful execution can be guaranteed is the `Pi1`, which at this point is 'a passed station'[^2].

[^2]: This is a *Dutchism*. It represents my sentiments perfectly.

----

## Create a bootable SD-card

	> rpi-imager

Select a minimal install for your Pi version, i.e. one without graphics support.

Select custom configuration:

- Enable `ssh` server
- Set a username and password for login

Put the SD-card in the PI, and give it current to start it up.

----

## VC4: Set memory split and remove dtoverlay[^3]

[^3]: It used to be possible to adjust the memory split witn `raspi-config`.
      This option has been removed in more recent `Raspbian` versions.

<!-- Line break in Markdown is "two or more spaces at the end of the line". -->

This is for Pi1, Pi2, Zero, and probably also Pi3.

`dtoverlay` should *not* be specified for Pi1.
In addition, this sets the memory split with `gpu_mem` for `vc4`.

	sudo vi  /boot/firmware/config.txt

Find the following lines:

	# Enable DRM VC4 V3D driver on top of the dispmanx display stack
	dtoverlay=vc4-fkms-v3d
	max_framebuffers=2

On `Pi Zero`, the text is:

	# Enable DRM VC4 V3D driver
	dtoverlay=vc4-kms-v3d
	max_framebuffers=2

Replace with:

	#dtoverlay=vc4-fkms-v3d
	max_framebuffers=2
	gpu_mem=128


To check the memory split:

	vcgencmd get_mem gpu

Reboot after the edit.

----

## Configuration on the Pi

In the following:

- replace `user` with your username
- replace `pi` with the hostname of the headless Pi you are installing
- replace `main` with the computer you are installing from


----

### Set automatic login for `pi`

From `main`:

	ssh-copy-id user@pi
	scp .vimrc user@pi:~    # Personal configuration
	ssh pi                  # Mainly to test login

----

### Set localization on `pi`

This is to remove the UK Pound sign from the keyboard.

There are other ways of doing this, this way is mainly my tactile memory.

On `pi`:

	sudo raspi-config

- Select 5. Localization Options
- Select Locale
- Unselect `en_GB.UTF-8 UTF-8`
- Select `en_US.UTF-8 UTF-8`
- \<Ok\>
- Again select `en_US.UTF-8`
- \<Ok\>
- \<Finish\>

----

### Installation on `pi`


	ssh-keygen
	ssh-copy-id user@pi

	sudo apt-get update
	sudo apt-get -y upgrade
	sudo apt-get install -y git libraspberrypi-dev raspberrypi-kernel-headers expat libexpat1-dev libz-dev ruby mc vim cmake libdrm-dev raspi-config

	# Do following separately, if it fails in previous due to unmet dependencies in previous step
	sudo apt-get install -y git libraspberrypi-dev
	sudo apt-get install -y git raspberrypi-kernel-headers

	# Had to do following explicitly on zero
	sudo apt-get install -y libdrm-dev
	sudo apt-get install -y ruby

	mkdir ~/projects
	mkdir ~/tmp

  # Following before cloning
  git config --global init.defaultBranch main

**TODO:** Adjust following URL's for `GitHub`

	cd projects
	git clone ssh://backup/home/git_masters/V3DLib
	git clone ssh://backup/home/git_masters/Granlund

	# For al git own repo's (previous steps)
	cd V3DLib
	git config --global user.email wrijnders@gmail.com
	git config --global user.name  "Wim Rijnders"
	git config pull.rebase false
	git config --global core.editor "vim"
	git push --set-upstream origin master

	cd extern/userland
	cmake .
	make
	cd ../../

Finally, you can build the library.

**Fair Warning:** The first build can take a *long* time, especially on older Pi's.

	make all runTests

	# To optionally run the unit tests:
	make test

----

#### Footnotes

