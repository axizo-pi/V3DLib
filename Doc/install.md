<head>
	<link rel="stylesheet" type="text/css" href="css/docs.css">
</head>

# Install Instructions

I'm sorry, I can't make this easy. You've got to jump through several hoops to make it work.

It is not possible to run the library on Pi's which have graphical output[^1]. The Pi needs to run **headless**.

[^1]: Not entirely true. The library will work *sometimes* if you're lucky. The only Pi on which
			successful execution can be guaranteed is the `Pi1`, which at this point is 'a passed station'[^2].

[^2]: This is a *Dutchism*. It represents my sentiments perfectly.

To recap, because they are referenced in this page:

| GPU           | Short | On Pi               |
| ------------- | ----- | ------------------- |
| VideoCore IV  | `vc4` | Pi1, Pi2, Pi3, Zero |
| VideoCore VI  | `vc6` | Pi4                 |
| VideoCore VII | `vc7` | Pi5                 |


----

## Create a bootable SD-card

- Attach an SD-card to your computer (assuming Pi) Then:

    > sudo rpi-imager

- Select your Pi
- Under _Raspberry PI OS (other)_ select **Raspbian Pi OS Lite (64 _or_ 32-bit)**.
  If 64-bit is available, select that.
- Select the SD-card you want to install on.
- _Choose hostname_: Your choice; I prefer using easily memorable names.
  for exampleA my Pi5's are called **pi5-1** and **pi5-2**.
- _Localisation_ and _Customisation_ should be 0bvious (I hope), except perhaps for:
  * _enable SSH_ - you need this. Initially, select _Use password authentication_
  * Don't bother with _Rapberry Pi Connect_ unless you're sure you want it.
- _Write_ and wait.

Are there errors during execution of `rpi-imager`?  Check **Note 1** below.

## Start the Pi up

Put the SD-card in the PI, and give it current to start it up.  
At this point, I check my router to see if the new Pi registers.

Try a regular login. Working? Yay!

You may get a messages about locale; see **Note 2**.  
If you don't get the message, change the locale anyway; see **Note 4**.

### Set automatic login for `pi`

Log out back to your main computer, which is hereby named `main`.  
Fill in `user` and `pi` for your situation.

From `main`:

	ssh-copy-id user@pi
	scp .vimrc user@pi:~    # Personal configuration, YMMV
	ssh pi                  # Test login


### Set memory split for `VideoCore IV`

Are you installing for `vc4`?
You need to disable the `dtoverlay` and set the memory split; see **Note 3**.

## Optional: run `sudo` without password

I do this on all my Pi's. I recommend this but you can think otherwise.

    > sudo visudo

Add Line:

    user ALL=(ALL) NOPASSWD:ALL


## Installation on `pi`

Login to `pi` and do following:

    ssh-keygen
    
    sudo apt-get update
    sudo apt-get -y upgrade
    
    sudo apt-get install -y git expat libexpat1-dev libz-dev ruby mc vim cmake libdrm-dev raspi-config gdb

    #
    # apt install - On pi only
    #
    # For Debian 13 (Trixie):
    sudo apt-get install -y git libdtovl0:armhf libdtovl0
    
    # For all other versions:
    sudo apt-get install -y git libraspberrypi-dev raspberrypi-kernel-headers
    # If previous fails due to unmet dependencies, do following separate steps: 
    sudo apt-get install -y git libraspberrypi-dev
    sudo apt-get install -y git raspberrypi-kernel-headers
    
    # Had to do following explicitly on zero
    sudo apt-get install -y libdrm-dev
    sudo apt-get install -y ruby

    mkdir ~/projects
    mkdir ~/tmp

    # Following before cloning
    git config --global init.defaultBranch main

    cd projects
    git clone https://github.com/axizo-pi/V3DLib.git
    git clone https://github.com/axizo-pi/CmdParameter.git

    # For al own git repo's (previous steps)
    cd V3DLib
    git config --global user.email wrijnders@gmail.com
    git config --global user.name  "Wim Rijnders"
    git config pull.rebase false
    git config --global core.editor "vim"
    #git push --set-upstream origin main  # Only if you have a github account

    # Create makefile for external project (not automated yet)
    cd extern/userland
    cmake .
    make
    cd ../../

Finally, you can build the library.

**Fair Warning:** The first build can take a *long* time, especially on older Pi's.


## Debian 13 (Trixie), `vc4` - install `bcm_host`

`bcm_host` is not available on `Debian 13 (Trixie)` and required for running on `vc4`.

Thankfully, it is part of the `userland` repository, and it has been incorporated into
the local `userland` instance. It gets built when compiling `userland`.

It still needs to be installed manually (`make install` doesn't work, the target directories are wrong).
To install, from the `V3DLib` base directory:

    > cd cd extern/userland/build/lib/
    > sudo cp libbcm_host.so /usr/lib/aarch64-linux-gnu/

## Run the unit tests

    > make all runTests
    > make test

## Notes

### 1. Error during `rpi-imager`

`rpi-imager v2.0.6`: Writing to _some_ sd-cards results in the following continuous error:

    [FileOps] io_uring short write: expected 0, got 4194304

This is a [https://forums.raspberrypi.com/viewtopic.php?t=397059](known error) and is fixed in `v2.0.7`.
I am anxiously waiting for the upgrade (supplied `AppImage` didn't work for me.

In the meantime, the best advice I can give you is: try another SD-card.

### 2. Local messages on startup

On at least Debian 13 (Trixie), you get the following message on initial login:

    bash: warning: setlocale: LC_ALL: cannot change locale (en_US.UTF-8): No such file or directory
    _____________________________________________________________________
    WARNING! Your environment specifies an invalid locale.
     The unknown environment variables are:
       LC_CTYPE=en_US.UTF-8 LC_MESSAGES=en_US.UTF-8 LC_ALL=en_US.UTF-8
     This can affect your user experience significantly, including the
     ability to manage packages. You may install the locales by running:

     sudo dpkg-reconfigure locales

     and select the missing language. Alternatively, you can install the
     locales-all package:

     sudo apt-get install locales-all

    To disable this message for all users, run:
       sudo touch /var/lib/cloud/instance/locale-check.skip
    _____________________________________________________________________

Run the first option:

     > sudo dpkg-reconfigure locales

And in the list select option `en_US.UTF-8 UTF-8`. And again.


## 3.  VC4: Set memory split and remove dtoverlay[^3]

[^3]: It used to be possible to adjust the memory split witn `raspi-config`.
      This option has been removed in more recent `Raspbian` versions.


This is for `Pi1`, `Pi2`, `Pi3` and `Zero`.

`dtoverlay` should *not* be specified for Pi1.
In addition, this sets the memory split with `gpu_mem`.

	sudo vi /boot/firmware/config.txt

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


Reboot after the edit.

To check the memory split:

	vcgencmd get_mem gpu


### 4. Set localization on `pi`

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

#### Footnotes

