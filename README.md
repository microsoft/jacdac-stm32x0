# JACDAC for STM32F0/STM32G0/...

You might have heard of [JACDAC](https://jacdac.org).
It's a new protocol that aims to standardize connecting microcontrollers dynamically,
and with very little wiring (one wire for data, one for GND, and optionally one for power).
One scenario is networking of MCU-based devices, for example to enable multiplayer
for your awesome [MakeCode Arcade](https://arcade.makecode.com) games.
Another is connecting peripherals to a host device (joystick anyone?!).

* [Preliminary notes on JACDAC v1](jacdac-v1-spec.md)

## TODO

* [x] implement reset counter and ACK flag in `AD[0]`
* [ ] consider thermal shutdown at 50C or so (assuming it's because of heat of some other component)
* [ ] use SI values for sensors with 16 bit scaling?
* [x] add CTRL cmds for time and software version (different than hw!)
* [x] the "combined" flashing doesn't work - figure out why
* [x] UF2 flashing fails ("misaligned" error)

## Release process

This repository uses [semantic release](https://github.com/semantic-release/semantic-release) to automatically create releases upon analyzing commits.

The commits can be formatted using https://github.com/angular/angular.js/blob/master/DEVELOPERS.md#-git-commit-guidelines.

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
