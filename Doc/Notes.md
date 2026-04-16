<head>
	<link rel="stylesheet" type="text/css" href="css/docs.css">
</head>

# Notes

## <a name="useful-commands">Useful Commands</a>

Values here are for `Pi5`, unless otherwise specified.

**Get GPU Speed**

`Pi5`:

    > vcgencmd get_config int | grep v3d
    v3d_freq=960
    v3d_freq_min=500

`Pi4`, `Zero`:

    > vcgencmd get_config int | grep gpu
    gpu_freq=500
    ...

`Pi3`:

    > sudo vcgencmd get_config int | grep gpu
    gpu_freq=300

Previous doesn't work on `Pi2`. Following is a tentative to get the QPU speed anyway:

    > sudo vcgencmd measure_clock core
    frequency(1)=250000000

Also works for `Pi1`.


**Get GPU info**

`Pi5`, `Pi4`. Among other info, number of QPU's.

    > cat /sys/kernel/debug/dri/0/v3d_ident
    Revision:   7.1.7.0
    MMU:        yes
    TFU:        no
    MSO:        yes
    L3C:        no (0kb)
    Core 0:
      Revision:     7.1
      Slices:       4
      TMUs:         4
      QPUs:         16
      Semaphores:   0

**Get CPU speed**

`lscpu` returns other stuff which may be useful, in particular `Core(s) per cluster`.

    > lscpu | grep CPU.*MHz
    CPU(s) scaling MHz:                   62%
    CPU max MHz:                          2400.0000
    CPU min MHz:                          1500.0000

Following works on `Pi2`, should be OK on other Pi's:

    > vcgencmd get_config int | grep freq
    arm_freq=900
    arm_freq_min=600
    ...

**Check distro**

    lsb_release -a

**Check 32/64 bits**

    getconf LONG_BIT

-------------------

## Mesa2 Investigations

### Regfile read/write

Remember: vc6 has one regfile, not a and b

| raddr\_a | read address from regfile a |
| raddr\_b | read address from regfile b |
| waddr\_a | write address to regfile a |
| waddr\_b | write address to regfile b |

- vc4: a/b's really read/write to regfile a/b
- vc6: all read/writes to the central regfile

**mux:**

mesa2, `src/broadcom/qpu/qpu_instr.h`:

    struct v3d_qpu_input {
      union {
        enum v3d_qpu_mux mux; /* V3D 4.x */
        uint8_t raddr; /* V3D 7.x */
      };
      enum v3d_qpu_input_unpack unpack;
    };

`mux` maps to fields `add_a/b` and `mul_a/b'.  
Mux values are removed in vc7. By implication, accumulator registers have been discarded.

`vc6`, both mesa libs:

Struct `v3d_qpu_input` used in `v3d_qpu_alu_instr`, part of  `v3d_qpu_instr`.
The latter contains `raddr_a/b`.

### Small imm's

mesa2, `src/broadcom/qpu/qpu_instr.h`, struct `v3d_qpu_sig`:

    ...
    bool small_imm_a:1; /* raddr_a (add a), since V3D 7.x */
    bool small_imm_b:1; /* raddr_b (add b) */
    bool small_imm_c:1; /* raddr_c (mul a), since V3D 7.x */
    bool small_imm_d:1; /* raddr_d (mul b), since V3D 7.x */
    ...

Previously, there was only one field `small_imm`; this maps to `small_imm_b`.
So in `mesa`, only `raddr_b` could be used for small imm.

In `mesa2`, for `V2D 7`, all raddr fields can be used for small_imm,
_but not at the same time_ (empirical observation).

-------------------

## Setting of Branch Conditions

**TODO:** Make this a coherent text.

Source: qpu_instr.h, line 74, enum v3d_qpu_uf:

How I interpret this:

- AND: if all bits set
- NOR: if no bits set
- N  : field not set
- Z  : Field zero
- N  : field negative set
- C  : Field negative cleared

What the bits are is not clear at this point.
These assumptions are probably wrong, but I need a starting point.

**TODO:** make tests to verify these assumptions (how? No clue right now)

So:

- vc4 `if all(nc)...` -> ANDC
- vc4 `nc` - negative clear, ie. >= 0
- vc4 `ns` - negative set,   ie.  < 0


## Handling privileges

In order to use the `VideoCore`, special privileges are required to access certain devices files.  The default way is to run the applications with `sudo`.

You might run into the following situation (e.g.):

     > obj-qpu/bin/detectPlatform 
    Detected platform: Raspberry Pi 2 Model B Rev 1.1
    Can't open device file: /dev/vcio
    Try creating a device file with: sudo mknod /dev/vcio c 100 0

The solution for this is to become a member of group `video`:

     > sudo useradd -g video <user>

Where you fill in  a relevant user name for `<user>`. To enable this, logout and login, or start a new shell.

Unfortunately, this solution will not work for access to `/dev/mem`. You will still need to run with `sudo` for any application that uses the `VideoCore` hardware.

-------------------

## Issues with Installation


### Debian 13 Trixie has no `bcm\_host`

This library is required for running `vc4` applications.  
The workaround is to build and install it yourself. The steps are incorporated
in the [Install Intructions](Install.md)

### 32-bits Debian 12 Bookworm:

* 64-bits appears to respond **much** faster. Debian 10 Buster 32-bits is fine BTW.

Gives following error:

    > sudo ./obj/qpu-debug/bin/Hello
    'mmap of bo 2 (offset 0x0000000100001000, size 8388608) failed
    Aborted'

* Debian 10 Buster 32-bits works
* Debian 12 Bookworm 32-bits doesn't. see above comment.
* From now on, we do not support 32-bits

### Raspbian screwed up

I followed apt-get's advice and soon chromium was crashing.
When I rebooted, the Raspbian GUI was replaced by Debian.
Thankfully, all programs still appear to work.  
[Possible reason](https://forums.raspberrypi.com/viewtopic.php?t=371573)

Another reason could be that I ran V3DLib accidentally on the GUI machine.
but that was hours ago.

### raspi-config does not support 'Memory Split' any more, not even on Pi1.

### Add user to group `video`

    > sudo usermod -a -G video wim

This supplants any previous instruction.


### Error while running ID after (first) restart:

    /obj/qpu-debug/bin/ID: error while loading shared libraries: libbcm_host.so.0: cannot open shared object file: No such file or directory

Fix:

    sudo apt-get update
    sudo apt-get install --reinstall libraspberrypi0 libraspberrypi-dev libraspberrypi-doc libraspberrypi-bin


-------------------

## Issues with Old Distributions and Compilers

Following is known to occur with `Raspbian wheezy`.

* Certain expected functions are not defined

Following prototypes are missing in in `/opt/vc/include/bcm\_host.h`:

  - `bcm\_host\_get\_peripheral\_address()`
  - `bcm\_host\_get\_peripheral\_size()`

-------------------

## Issues with Installation

### Usage of a local dynamic library on execution:

Used when testing `libbcm\_host.so`.

    > export LD_LIBRARY_PATH=~/projects/V3DLib/extern/userland/build/lib/
    > ls $LD_LIBRARY_PATH   # Just checking
    > alias sudo='sudo PATH="$PATH" HOME="$HOME" LD_LIBRARY_PATH="$LD_LIBRARY_PATH"'

This fails when running `make test` in two ways.
My solution is to just install the library in the system library directory.
