# Jacdac for STM32F0xx and STM32G0xx

This repository contains firmware for [Jacdac](https://aka.ms/jacdac) modules based on STM32F0xx and STM32G0xx
chips and is part of the [Jacdac Module Development Kit](https://github.com/microsoft/jacdac-mdk).

(The STM32G0xx support is still a work in progress.)

## Bootloader

This repository contains both the firmware for running services (eg., accelerometer service) on a Jacdac module,
and a bootloader which allows for updating the firmware using the Jacdac protocol.
This update process can be performed by the user from the [Jacdac website](https://microsoft.github.io/jacdac-ts/tools/updater)
(while developing firmware you will typically use a debugger to deploy both the bootloader and the firmware).

## Building

The top-level repo to build is [jacdac-msr-modules](https://github.com/microsoft/jacdac-msr-modules).
It imports as submodules [this repo (jacdac-stm32x0)](https://github.com/microsoft/jacdac-stm32x0) 
and [jacdac-c](https://github.com/microsoft/jacdac-c)
(which contains platform-independent code implementing Jacdac services, as well as various I2C drivers).

When building your own firmware, you will need to copy (or fork) `jacdac-msr-modules`.
All the instructions below use `jacdac-msr-modules` (or its copy) as the root folder.
You typically will not need to fork `jacdac-stm32x0` nor `jacdac-c`.

The build instructions are here, and not in `jacdac-msr-modules`, to avoid them getting stale in its various forks.

You will need a Unix-like environment to build the firmware (see below for instructions on how to get that on Windows).

* install `arm-none-eabi-gcc` (we've been using `9-2019-q4-major`);
  please note that `gcc-arm-none-eabi` that comes with Ubuntu won't work
* install `openocd` (optional when using Black Magic Probe)
* install node.js (some linux distros have old versions of Node; get at least 14.5.2 from [NodeSource](https://github.com/nodesource/distributions/blob/master/README.md))
* install GNU Make
* run `make`; you should get a successful build

Upon first run of `make`, a `Makefile.user` file will be created.
You will want to adjust the settings in there - there are comments in there that should guide you through the process.

To deploy the firmware to a module you will need a debugger interface.
You have three options:
* [Black Magic Probe](https://github.com/blacksphere/blackmagic/wiki); you can also re-program other debuggers with BMP firmware
* a CMSIS-DAP debugger; we've been using [Particle Debugger](https://store.particle.io/products/particle-debugger)
* an ST-LINK/V2 or one of its clones
You will want to set the right interface in `Makefile.user`.

Following commands can be used to deploy firmware:
* `make run BL=1` - deploy bootloader
* `make run` - deploy firmware
* `make full-flash` - deploy both bootloader and firmware
* `make flash-loop` - run flashing process in a loop - you can flash multiple devices in a row this way

Aliases:
* `make r` for `make run`
* `make l` for `make flash-loop`
* `make ff` for `make full-flash`

Other than the building/deployment targets, the following might be of note:

* `make st` - print RAM/flash stats for the current firmware
* `make stf` - same, but break it up by function, not only file
* `make gdb` - run GDB debugger
* `make clean` - clean (duh!)
* `make drop` - build all firmware images specified in `DROP_TARGETS`

## Building on Windows

You will need a Unix-like environment to build the firmware.
Luckily, you most likely already have one - it comes with Git for Windows.
If you do not have [Git for Windows](https://git-scm.com/download/win) yet, install it.
Then you will need to install GNU Make:

* download GNU Make (without guile) from [EzWinPorts](https://sourceforge.net/projects/ezwinports/files/)
* open Administrator command prompt
* run `cd "\Program Files\Git\usr"`
* run `bin\unzip.exe c:\Users\<you>\Downloads\make-4.3-without-guile-w32-bin.zip` replacing `<you>` with your user name

Head to Start -> Git Bash. When you type `make` you should now see
`make: *** No targets specified and no makefile found.  Stop.`.

Now install [GNU Arm Embedded Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads)
version `9-2019-q4-major` (do not use other versions for now).
Get the `.exe` installer, and agree to adding the tools to your `PATH` variable. If the installer fails to add to `PATH`, then do so yourself, adding:
`C:\Program Files (x86)\GNU Tools Arm Embedded\9 2019-q4-major\bin`

You will also need to [install node.js](https://nodejs.org/en/download/) - take the Windows `.msi` 64 bit installer.

If you run Git Bash again (it has to be restarted to see change in `PATH`), and type 
`arm-none-eabi-gcc --version` you should see something about `9-2019-q4-major`,
and if you type `node --version` you should get its version number.

If using Black Magic Probe, connect it to your computer.
You should see two COM ports that correspond to it in Device Manager (for example `COM7` and `COM8`)
You will want to use the lower numbered one (for example `COM7`).

If using stlink or cmsis/dap, you'll need to install openocd **(TODO: this currently doesn't work)**:

* get [7-Zip decompressor](https://www.7-zip.org/)
* get openocd `.7z` archive [from GNU Toolchains](https://gnutoolchains.com/arm-eabi/openocd/)
* extract `bin` subfolder of that archive to `C:\Program Files\Git\usr\bin`,
  `share` to  `C:\Program Files\Git\usr\share` etc.

If you want to extract somewhere else, make sure to add the `bin` somefolder to `PATH`.

Now, follow the usual build instructions above.

Note: using WSL2 instead of the bash shell etc coming with Git 
is not recommended since it cannot access USB for deployment and debugging.

## Adding new modules

* copy the `jacdac-msr-modules` repo, into (say) `jacdac-acme-corp-modules`
* replace string `jacdac-msr-modules` with `jacdac-acme-corp-modules` in `package.json`
* copy `targets/_example/` to `targets/acme-corp/` (replaceing `acme-corp` with the name of the series of modules)
* edit [targets/acme-corp/board.h](targets/_example/board.h) to match your module
* you likely do not need to edit [targets/acme-corp/config.mk](targets/_example/config.mk), even if using
  a beefier MCU from the F03x family - they should be backward-compatible
* edit [targets/acme-corp/profile/module.c](targets/_example/profile/module.c) 
  to include your module name and used services (follow comments in `module.c`);
  see [jd_service_initializers.h](https://github.com/microsoft/jacdac-c/blob/master/services/interfaces/jd_service_initializers.h)
  for list of services
* rename `module.c` to match the type of module (eg. `servo.c`)
* if you have several modules with non-conflicting `board.h` definitions,
  you can create more files under `targets/acme-corp/profile/`;
  otherwise you'll need to create `targets/acme-corp-2` or something similar
* edit `Makefile.user` to set `TRG`, eg. `TRG = acme-corp servo`
* run `make`; this will generate a new unique identifier and place as an argument of `FIRMWARE_IDENTIFIER` macro
* make sure to never change the firmware identifier number, as that will break future firmware updates

If you copy `targets/jm-*/profiles/something.c` to start your own module, remember to rename it, and
set the `FIRMWARE_IDENTIFIER` to `0` (the one in `targets/_examples` already has it set to `0`).
This way, the build process will generate a new firmware identifier.

## Adding new services

This topic is [covered in jacdac-c](https://github.com/microsoft/jacdac-c#adding-new-services).

## Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.opensource.microsoft.com.

When you submit a pull request, a CLA bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., status check, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
