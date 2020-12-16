# JACDAC for STM32F0xx

This repository contains firmware for [JACDAC](https://aka.ms/jacdac) modules
based on STM32F0xx chips and is part of [JACDAC Module Development Kit](https://github.com/microsoft/jacdac-mdk).

## Bootloader

This repo contains both the firmware for running services (eg., accelerometer service) on your modules,
and also a bootlaoder which allows for updating the firmware using JACDAC protocol.
This update process can be performed by the user from the [JACDAC website](https://microsoft.github.io/jacdac-ts/tools/updater).
However, while developing firmware you typically use a debugger to deploy both the bootloader and the firmware.

## Building

You will need a Unix-like environment to build the firmware.
On Windows, you can use Windows Subsystem for Linux or mingw32.

* install `arm-none-eabi-gcc` (we've been using `9-2019-q4-major`)
* install `openocd` (optional when using Black Magic Probe)
* install node.js
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
* edit `targets/acme-corp/board.h` to match your module
* edit `targets/acme-corp/profiles/module.c` to include your module name and used services
  (follow comments in `module.c`)
* rename `module.c` to match the type of module (eg. `servo.c`)
* if you have several modules with non-conflicting `board.h` definitions,
  you can create more files under `targets/acme-corp/profiles/`
* edit `Makefile.user` to set `TRG`, eg. `TRG = acme-corp servo`
* run `make`; this will update the number after `FIRMWARE_IDENTIFIER` - it's a hash of your module name
* make sure to never rename your device, as that will break future firmware updates


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
