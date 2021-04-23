# Jacdac for STM32F0xx and STM32G0xx

This repository contains firmware for [Jacdac](https://aka.ms/jacdac) modules based on STM32F0xx and STM32G0xx
chips and is part of the [Jacdac Module Development Kit](https://github.com/microsoft/jacdac-mdk).

## Bootloader

This repository contains both the firmware for running services (eg., accelerometer service) on a Jacdac module,
and a bootloader which allows for updating the firmware using the Jacdac protocol.
This update process can be performed by the user from the [Jacdac website](https://microsoft.github.io/jacdac-docs/tools/updater)
(while developing firmware you will typically use a debugger to deploy both the bootloader and the firmware).

## Development

The top-level repo to build is [jacdac-msr-modules](https://github.com/microsoft/jacdac-msr-modules).
It imports as submodules [this repo (jacdac-stm32x0)](https://github.com/microsoft/jacdac-stm32x0) 
and [jacdac-c](https://github.com/microsoft/jacdac-c)
(which contains platform-independent code implementing Jacdac services, as well as various I2C drivers).

When building your own firmware, you will need to create your own repository from `jacdac-msr-modules` template.
You can do it by following this link https://github.com/microsoft/jacdac-msr-modules/generate
or using the green "Use this template" button in top-right corner at https://github.com/microsoft/jacdac-msr-modules
If you're just playing around, you can simply clone `jacdac-msr-modules`.

All the instructions below use your copy of `jacdac-msr-modules` as the root folder.
You typically will not need to fork `jacdac-stm32x0` nor `jacdac-c`.

The build instructions are here, and not in `jacdac-msr-modules`, to avoid them getting stale in its various copies.


## Setup

You will need a Unix-like environment to build the firmware (see below for instructions on how to get that on Windows, macOS, and Linux) and the following tools:

* **arm-none-eabi-gcc** (we've been using `9-2019-q4-major` and recently switched to `10-2020-q4-major`)
* **node.js**
* **GNU Make**

### macOS

* **install arm-gcc**: There are verification errors with brew packages for arm-gcc, so we recommend
[downloading the -mac.pkg version from Arm](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads/product-release).

* **setup path**: Once you install it, you either need to add `/Applications/ARM/bin` to your `PATH`, or symlink binaries like this:

```
cd /usr/local/bin; ln -sf /Applications/ARM/bin/arm-none-eabi-* .
```

* **install Node.js** Node.js can be [downloaded from the usual place](https://nodejs.org/en/download/).

GNU Make comes with macOS.

### Linux

* please note that `gcc-arm-none-eabi` that comes with Ubuntu won't work; we recommend you download it from Arm
* some linux distros have old versions of Node; get at least 14.5.2 from [NodeSource](https://github.com/nodesource/distributions/blob/master/README.md)
* GNU Make is almost always pre-installed

### Windows

You will need a Unix-like environment to build the firmware.
Luckily, you most likely already have one - it comes with Git for Windows.
If you do not have [Git for Windows](https://git-scm.com/download/win) yet, install it.
Then you will need to install GNU Make:

* download GNU Make (without guile) from [EzWinPorts](https://sourceforge.net/projects/ezwinports/files/)
* open Administrator command prompt
* run `cd "\Program Files\Git\usr"`
* run `bin\unzip.exe c:\Users\<you>\Downloads\make-4.3-without-guile-w32-bin.zip` replacing `<you>` with your user name

* Head to Start -> Git Bash. When you type `make` you should now see
`make: *** No targets specified and no makefile found.  Stop.`.

* install [GNU Arm Embedded Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads)
version `10-2020-q4-major` or `9-2019-q4-major` (do not use other versions for now).

* Get the `.exe` installer, and agree to adding the tools to your `PATH` variable. If the installer fails to add to `PATH`, then do so yourself, adding:
`C:\Program Files (x86)\GNU Tools Arm Embedded\10 2020-q4-major\bin`

* You will also need to [install node.js](https://nodejs.org/en/download/) - take the Windows `.msi` 64 bit installer.

If you run Git Bash again (it has to be restarted to see change in `PATH`), and type 
`arm-none-eabi-gcc --version` you should see something about `10-2020-q4-major`,
and if you type `node --version` you should get its version number.

If using Black Magic Probe, connect it to your computer.
You should see two COM ports that correspond to it in Device Manager (for example `COM7` and `COM8`)
You will want to use the lower numbered one (for example `COM7`).

Now, follow the usual build instructions above.

Note: using WSL2 instead of the bash shell etc coming with Git 
is not recommended since it cannot access USB for deployment and debugging.

## Building firmware

Run `make`; you should get a successful build.

Upon first run of `make`, a `Makefile.user` file will be created.
You will want to adjust the settings in there - there are comments in there that should guide you through the process.

## Deploying firmware

To deploy the firmware to a module you will need a debugger interface.
We've had best success with [Black Magic Probe](https://github.com/blacksphere/blackmagic/wiki) -
it can connect to the MCU even while it's sleeping (provided it wakes up every now and then).

Other debuggers generally require a reset before connection in that case.
There is some support for CMSIS-DAP and ST-LINK/V2 through `openocd`, but we do not recommend using
it due to the sleep issue.

You can convert a [$3 Bluepill board into Black Magic Probe](https://github.com/mmoskal/blackmagic-bluepill).

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

## Adding new modules

* [create a new repo](https://github.com/microsoft/jacdac-msr-modules/generate) from `jacdac-msr-modules`;
  let's say the new repo is called `jacdac-acme-corp-modules`
* replace string `jacdac-msr-modules` with `jacdac-acme-corp-modules` in `package.json`
note: there is no package.json
* copy `targets/_example/` to `targets/acme-corp/` (replaceing `acme-corp` with the name of the series of modules)
* edit [targets/acme-corp/board.h](targets/_example/board.h) to match your module
* you likely do not need to edit [targets/acme-corp/config.mk](targets/_example/config.mk), even if using
  a beefier MCU from the F03x family - they should be backward-compatible
* edit [targets/acme-corp/profile/module.c](targets/_example/profile/module.c) 
  to include your module name and used services (follow comments in `module.c`);
  see [jd_services.h](https://github.com/microsoft/jacdac-c/blob/master/services/jd_services.h)
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

Now, edit `DROP_TARGETS` in `Makefile` to only include your `acme-corp` folder
(and in future `acme-corp-v2` etc.).
You don't need to build `jm-*` modules, so remove them from `DROP_TARGETS`.

When you run `make drop` now, you should get a `.uf2` file combining firmware for all your modules.

## Adding new services and drivers

This topic is [covered in jacdac-c](https://github.com/microsoft/jacdac-c#adding-new-services).
When adding services or drivers, you can put them in `addons/` folder of your modules repo,
or submit them as PRs in `jacdac-c`.

## Release process

Use `make bump` to create a new release.
This will ask you for a version number (providing a default that just updates the patch number),
update `CHANGES.md`, create a git tag, and push it to the origin.

If the `jacdac-acme-corp-modules` repo is public on github, the github action will create a new binary
file and place it under `dist/fw-VERSION.uf2`, so there's nothing else to do.

If you want to build by hand, run `make drop` after `make bump` and copy `built/fw-VERSION.uf2`
to `dist/fw-VERSION.uf2` in your release repo.
You should probably also copy over `CHANGES.md`, so your users have some idea of what it being updated.

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
