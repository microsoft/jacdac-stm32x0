# JACDAC for STM32F0xx

This repository contains firmware for [JACDAC](https://aka.ms/jacdac) modules based on STM32F0xx chips and is part of the [JACDAC Module Development Kit](https://github.com/microsoft/jacdac-mdk).

## Bootloader

This repository contains both the firmware for running services (eg., accelerometer service) on a JACDAC module,
and a bootloader which allows for updating the firmware using the JACDAC protocol.
This update process can be performed by the user from the [JACDAC website](https://microsoft.github.io/jacdac-ts/tools/updater)
(while developing firmware you will typically use a debugger to deploy both the bootloader and the firmware).

## Building

You will need a Unix-like environment to build the firmware (see below for instructions on how to get that on Windows).

* install `arm-none-eabi-gcc` (we've been using `9-2019-q4-major`);
  please note that `gcc-arm-none-eabi` that comes with Ubuntu won't work
* install `openocd` (optional when using Black Magic Probe)
* install node.js (some linux distros have old versions of Node; get at least 14.5.2 from https://github.com/nodesource/distributions/blob/master/README.md) 
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

## Building on Windows

You will need a Unix-like environment to build the firmware.
Luckily, you most likely already have one - it comes with Git for Windows.
If you do not have [Git for Windows](https://git-scm.com/download/win) yet, install it.
Then you will need to install GNU Make:

* download GNU Make (without guile) from https://sourceforge.net/projects/ezwinports/files/
* open Administrator command prompt
* run `cd "\Program Files\Git\usr"`
* run `bin\unzip.exe c:\Users\<you>\Downloads\make-4.3-without-guile-w32-bin.zip` replacing `<you>` with your user name

Head to Start -> Git Bash. When you type `make` you should now see
`make: *** No targets specified and no makefile found.  Stop.`.

Now install [GNU Arm Embedded Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads)
version `9-2019-q4-major` (do not use other versions for now).
Get the `.exe` installer, and agree to adding the tools to your `PATH` variable. If the installer fails to add to `PATH`, then manual do so yourself, adding:
* "C:\Program Files (x86)\GNU Tools Arm Embedded\9 2019-q4-major\bin"

You will also need to [install node.js](https://nodejs.org/en/download/) - take the Windows `.msi` 64 bit installer.

If you run Git Bash again (it has to be restart to see change in `PATH`), and type 
`arm-none-eabi-gcc --version` you should see something about `9-2019-q4-major`,
and if you type `node --version` you should get its version number.

If using Black Magic Probe, connect it to your computer.
You should see two COM ports that correspond to it in Device Manager (for example `COM3` and `COM4`)
You will want to use the lower numbered one (for example `COM3`).

If using stlink or cmsis/dap, you'll need to install openocd (this currently doesn't work):

* get [7-Zip decompressor](https://www.7-zip.org/)
* get openocd `.7z` archive [from GNU Toolchains](https://gnutoolchains.com/arm-eabi/openocd/)
* extract `bin` subfolder of that archive to `C:\Program Files\Git\usr\bin`,
  `share` to  `C:\Program Files\Git\usr\share` etc.

If you want to extract somewhere else, make sure to add the `bin` somefolder to `PATH`.

Now, follow the usual build instructions above.

Note: using WSL2 instead of the bash shell etc coming with Git 
is not recommended since it cannot access USB for deployment and debugging.


### Notable make targets

Other than the building/deployment targets, the following might be of note:

* `make st` - print RAM/flash stats for the current firmware
* `make stf` - same, but break it up by function, not only file
* `make gdb` - run GDB debugger
* `make clean` - clean (duh!)
* `make drop` - build all firmware images specified in `DROP_TARGETS`

## Adding new modules

* fork this repo
* copy `targets/_example/` to `targets/acme-corp/`
* edit [targets/acme-corp/board.h](targets/_example/board.h) to match your module
* you likely do not need to edit [targets/acme-corp/config.mk](targets/_example/config.mk), even if using
  a beefier MCU from the F03x family - they should be backward-compatible
* edit [targets/acme-corp/profile/module.c](targets/_example/profile/module.c) 
  to include your module name and used services (follow comments in `module.c`);
  see [jd_service_initializers.h](jacdac-c/services/interfaces/jd_service_initializers.h) for list of services
* rename `module.c` to match the type of module (eg. `servo.c`)
* if you have several modules with non-conflicting `board.h` definitions,
  you can create more files under `targets/acme-corp/profile/`
* edit `Makefile.user` to set `TRG`, eg. `TRG = acme-corp servo`
* run `make`; this will generate a new unique identifier and place as an argument of `FIRMWARE_IDENTIFIER` macro
* make sure to never change the firmware identifier number, as that will break future firmware updates

## Adding new services

This topic is [covered in jacdac-c](https://github.com/microsoft/jacdac-c#adding-new-services).

## TODO

* [ ] consider thermal shutdown at 50C or so (assuming it's because of heat of some other component)

## Release process

This repository uses [semantic release](https://github.com/semantic-release/semantic-release) to automatically create releases upon analyzing commits.

The commits can be formatted using https://github.com/angular/angular.js/blob/master/DEVELOPERS.md#-git-commit-guidelines.

* ``fix: some fix``, create a patch release
* ``feat: some feature``, create a minor release

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
